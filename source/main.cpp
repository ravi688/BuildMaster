#include <iostream>
#include <cstdlib>
#include <format>
#include <iterator>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <reproc/run.h>

// NOTE: Following defines are Automatically Defined by the Build System (meson.build)
// BUILDMASTER_VERSION_MAJOR
// BUILDMASTER_VERSION_MINOR
// BUILDMASTER_VERSION_MICRO
// MESON_BUILD_TEMPLATE_PATH

#define BUILDMASTER_VERSION_ENCODE(major, minor, micro) (((major) * 10000) + ((minor) * 100) + ((micro) * 1))
#define BUILDMASTER_VERSION BUILDMASTER_VERSION_ENCODE(BUILDMASTER_VERSION_MAJOR, BUILDMASTER_VERSION_MINOR, BUILDMASTER_VERSION_MICRO)

#define BUILDMASTER_VERSION_XSTRINGIZE(major, minor, micro) #major"."#minor"."#micro
#define BUILDMASTER_VERSION_STRINGIZE(major, minor, micro) BUILDMASTER_VERSION_XSTRINGIZE(major, minor, micro)
#define BUILDMASTER_VERSION_STRING BUILDMASTER_VERSION_STRINGIZE(BUILDMASTER_VERSION_MAJOR, BUILDMASTER_VERSION_MINOR, BUILDMASTER_VERSION_MICRO)

using json = nlohmann::ordered_json; 
// using json = nlohmann::json;

static constexpr std::string_view gBuildMasterJsonFilePath = "build_master.json";
static constexpr std::string_view gMesonBuildScriptFilePath = "meson.build";
static constexpr std::string_view gPython = "python3";
static constexpr std::string_view gMesonExecutableName = "build_master_meson";
// If on windows, use where instead of which
#ifdef _WIN32
static constexpr std::string_view gWhichCmd = "where";
// Otherwise use which (on linux and freebsd)
#else
static constexpr std::string_view gWhichCmd = "which";
#endif


// directoryBase: The base directory against which the the final path need to be calculated
// relativeDir: The path relative to the base directory passed as 'directoryBase' (first arg)
static std::string GetPathStrRelativeToDir(std::string_view directoryBase, std::string_view relativePath)
{
	auto buildMasterJsonFilePath = std::filesystem::path(directoryBase) / std::filesystem::path(relativePath);
	return buildMasterJsonFilePath.string();
}

// Use this function whenever you meant to get gBuildMasterJsonFilePath.
// DO NOT use gBuildMasterJsonFilePath directly as that would not consider the --directory flag 
static std::string GetBuildMasterJsonFilePath(std::string_view directory = ".")
{
	return GetPathStrRelativeToDir(directory, gBuildMasterJsonFilePath);
}

// Use this function whenever you meant to get gMesonBuildScriptFilePath.
// DO NOT use gMesonBuildScriptFilePath directory as that would not consider the --directory flag 
static std::string GetMesonBuildScriptFilePath(std::string_view directory = ".")
{
	return GetPathStrRelativeToDir(directory, gMesonBuildScriptFilePath);
}

// directory: value passed to --directory flag
static bool IsRegenerateMesonBuildScript(std::string_view directory)
{
	if(std::filesystem::exists(GetMesonBuildScriptFilePath(directory)))
	{
		auto mesonBuildScriptTime = std::filesystem::last_write_time(GetMesonBuildScriptFilePath(directory));
		auto buildMasterJsonTime = std::filesystem::last_write_time(GetBuildMasterJsonFilePath(directory));
		return buildMasterJsonTime >= mesonBuildScriptTime;
	}
 	return true;
}

static std::string LoadTextFile(std::string_view filePath)
{
	std::ifstream stream(filePath.data());
	if(!stream.is_open())
	{
		std::cerr << "Error: Failed to open " << filePath << "\n";
		exit(EXIT_FAILURE);
	}
    std::string line;
    std::string contents;
    while (std::getline(stream, line)) {
        contents.append(line);
        contents.append("\n");
    }
    return contents;	
}

static std::optional<std::pair<std::size_t, std::size_t>> IsBlankLine(const std::string& str, std::size_t charIndex)
{
	std::size_t lineBegin;
	while(charIndex > 0)
	{
		char ch = str[--charIndex];
		if(ch == '\n')
		{
			lineBegin = charIndex;
			break;
		}
		if(!std::isspace(ch))
			return { };
	}
	std::size_t lineEnd;
	while(charIndex < str.size())
	{
		char ch = str[charIndex++];
		if(ch == '\n')
		{
			lineEnd = charIndex;
			break;
		}
		if(!std::isspace(ch))
			return { };
	}
	return { { lineBegin, lineEnd } };
}

static void EraseCppComments(std::string& str)
{
	std::size_t index;
	while((index = str.find("//")) != std::string::npos)
	{
		auto newLineIndex = str.find_first_of('\n', index + 2);
		str.erase(std::next(str.begin(), index), std::next(str.begin(), newLineIndex));
		if(auto result = IsBlankLine(str, index); result.has_value())
			str.erase(std::next(str.begin(), result->first), std::next(str.begin(), result->second));
	}
}

// directory: value passed to --directory flag
static json ParseBuildMasterJson(std::string_view directory)
{
	std::string jsonStr = LoadTextFile(GetBuildMasterJsonFilePath(directory));
	EraseCppComments(jsonStr);
	std::cout << jsonStr << "\n";
	json data = json::parse(jsonStr);
	return data;
}

static std::string LoadMesonBuildScriptTemplate()
{
	std::string_view str = MESON_BUILD_TEMPLATE_PATH;
	std::cout << "MESON_BUILD_TEMPLATE_PATH: " << str << "\n";
	return LoadTextFile(str);
}

using TokenTransformCallback = std::function<std::string(std::string_view)>;

static std::string single_quoted_str(std::string_view str)
{
	return std::format("'{}'", str);
}

// Examples: 
// link_dir: $cuda_lib_path -> '-L' + cuda_lib_path
// link_dir: 'my/path/to/lib' -> '-L' + 'my/path/to/lib'
// link_dir: $cuda_lib_path + '/windows_lib_path/' -> '-L' + cuda_lib_path + '/windows/lib_path/'
// link_dir: $vulkan_sdk_path + $relative_lib_path -> '-L' + vulkan_sdk_path + relative_lib_path
// $vulkan_sdk_path -> vulkan_sdk_path
// my/path/to/vulkan/sdk -> 'my/path/to/vulkan/sdk'
static std::string ApplyMetaInfo(std::string_view str)
{
	std::string copyStr { str };
	std::size_t index = 0;
	bool isDollarSignFound = false;
	while(true)
	{
		index = copyStr.find_first_of("$", index);
		if(index == std::string::npos)
			break;
		copyStr.erase(index, 1);
		isDollarSignFound = true;
	}
	const std::string_view linkDirStr = "link_dir:";
	if(copyStr.find(linkDirStr) != std::string::npos)
	{
		std::string linkMetaStr = "'-L' + ";
		// Only literal linker lookup paths must be relative to project's source root
		// The reason we don't do this for paths specified by variables is that variables would generally
		// originate out of shell environment variables (like CUDA_PATH or VK_SDK_PATH), and these are already absolute paths.
		// If one needs to specify a literal indirectly (via a variable) then he must add `meson.project_source_root() + '/'` himself explicitly.
		if(!isDollarSignFound)
			linkMetaStr.append("meson.project_source_root() + '/' + ");
		copyStr.replace(0, linkDirStr.length(), linkMetaStr);
	}
	// If neither `link_dir: ` is found, nor $ is found, then we just wrap the str in single quotes
	else if(!isDollarSignFound)
		return single_quoted_str(str);
	return copyStr;
}

static void ProcessStringListElements(const json& jsonObj, std::ostringstream& stream, std::string_view delimit = " ", std::optional<TokenTransformCallback> callback = { })
{
	for(std::size_t i = 0; const auto& value : jsonObj)
	{
		const auto& str = value.template get<std::string>();
		std::string token = ApplyMetaInfo(str);
		if(callback)
			token = (*callback)(token);
		stream << token;
		if(++i < jsonObj.size())
			stream << "," << delimit;
	}
}

static void ProcessStringListElements(const json& jsonObj, std::string_view keyName, std::ostringstream& stream, std::string_view delimit = " ", std::optional<TokenTransformCallback> callback = { })
{
	auto it = jsonObj.find(keyName);
	if(it == jsonObj.end())
		return;
	ProcessStringListElements(it.value(), stream, delimit, callback);
}

static std::string GetListStringOrEmpty(const json& jsonObj, std::string_view keyName, std::string_view delimit = " ")
{
	std::ostringstream stream;
	ProcessStringListElements(jsonObj, keyName, stream, delimit);
	return stream.str();
}

static void SubstitutePlaceholder(std::string& str, std::string_view placeholderName, std::string_view substitute)
{
	auto it = str.find(placeholderName);
	if(it != std::string::npos)
		str.replace(it, placeholderName.size(), substitute);
}

template<typename T, typename ReturnType, typename... Args>
concept Callable = requires(const T& callable)
{
	{ callable(std::declval<Args>()...) } -> std::convertible_to<ReturnType>;
};

template<Callable<std::string> Callback>
static void SubstitutePlaceholder(std::string& str, std::string_view placeholderName, const Callback& callback)
{
	auto it = str.find(placeholderName);
	if(it != std::string::npos)
	{
		std::string substitute = callback();
		str.replace(it, placeholderName.size(), substitute);
	}
}

enum class TargetType
{
	StaticLibrary,
	SharedLibrary,
	HeaderOnlyLibrary,
	Executable
};

static TargetType DetectTargetType(const json& targetJson)
{
	auto it = targetJson.find("is_executable");
	if(it != targetJson.end() && it.value().template get<bool>())
		return TargetType::Executable;
	it = targetJson.find("is_static_library");
	if(it != targetJson.end() && it.value().template get<bool>())
		return TargetType::StaticLibrary;
	it = targetJson.find("is_shared_library");
	if(it != targetJson.end() && it.value().template get<bool>())
		return TargetType::SharedLibrary;
	it = targetJson.find("is_header_only_library");
	if(it != targetJson.end() && it.value().template get<bool>())
		return TargetType::HeaderOnlyLibrary;
	return TargetType::Executable;
}

template<typename T>
static std::optional<T> GetJsonKeyValueOrNull(const json& jsonObj, std::string_view key)
{
	if(auto it = jsonObj.find(key); it != jsonObj.end())
		return { it.value().template get<T>() };
	return { };
}

template<typename T>
static T GetJsonKeyValue(const json& jsonObj, std::string_view key, std::optional<T> defaultValue = {})
{
	if(auto result = GetJsonKeyValueOrNull<T>(jsonObj, key); result.has_value())
		return result.value();
	if(!defaultValue.has_value())
		throw std::runtime_error(std::format("No such key: {} exists in the json provided, and default value is not given either", key));
	return std::move(defaultValue.value());
}

static void ProcessStringList(const json& jsonObj, std::ostringstream& stream, std::string_view delimit = " ", std::optional<TokenTransformCallback> callback = { })
{
	stream << "[\n";
	ProcessStringListElements(jsonObj, stream, delimit, callback);
	stream << "\n";
	stream << "]\n";
}

static void ProcessStringList(const json& jsonObj, std::string_view keyName, std::ostringstream& stream, std::string_view delimit = " ", std::optional<TokenTransformCallback> callback = { })
{
	stream << "[\n";
	ProcessStringListElements(jsonObj, keyName, stream, delimit, callback);
	stream << "\n";
	stream << "]\n";
}

static void ProcessStringListDeclare(const json& targetJson, std::ostringstream& stream, std::string_view listName, std::string_view suffix, std::optional<TokenTransformCallback> callback = { })
{
	std::string name = GetJsonKeyValue<std::string>(targetJson, "name");
	stream << name << suffix << " = ";
	ProcessStringList(targetJson, listName, stream, "\n", callback);
}

static constexpr std::string_view GetTargetTypeStr(TargetType targetType)
{
	switch(targetType)
	{
		case TargetType::StaticLibrary : return "static_library";
		case TargetType::SharedLibrary : return "shared_library";
		case TargetType::Executable : return "executable";
		case TargetType::HeaderOnlyLibrary: assert(false && "HeaderOnlyLibrary wasn't expected here");
	}
	return "executable";
}

struct ProjectMetaInfo
{
	std::string name;
	std::string description;
};

struct VarSuffixData
{
	std::string_view sources;
	std::string_view dependencies;
	std::string_view linkArgs;
	std::string_view buildDefines;
	std::string_view useDefines;
	std::string_view includeDirs;
};

static void ProcessTarget(const json& targetJson, 
							std::ostringstream& stream,
							TargetType targetType,
							ProjectMetaInfo& projMetaInfo,
							VarSuffixData& suffixData)
{
	std::string name = GetJsonKeyValue<std::string>(targetJson, "name");
	// Libraries have is_install set to true by default
	// Executables have is_install set to false by default
	bool isInstall = GetJsonKeyValue<bool>(targetJson, "is_install", (targetType == TargetType::Executable) ? false : true);
	if(targetType != TargetType::HeaderOnlyLibrary)
	{
		std::string_view targetTypeStr = GetTargetTypeStr(targetType);
		stream << std::format("{} = {}('{}'", name, targetTypeStr, name);
		stream << std::format(",\n\t{}{} + sources_bm_internal__", name, suffixData.sources);
		stream << std::format(",\n\tdependencies: dependencies_bm_internal__ + {}{}", name, suffixData.dependencies);
		// NOTE: include_directies([...]) + include_directories([...]) is not possible in meson
		// So we need to use arrays to combine them
		stream << std::format(",\n\tinclude_directories: [inc_bm_internal__, {}{}]", name, suffixData.includeDirs);
		stream << std::format(",\n\tinstall: {}", isInstall ? "true" : "false");
		if(targetType != TargetType::Executable)
		{
			stream << ",\n\tinstall_dir: lib_install_dir_bm_internal__";
			stream << std::format(",\n\tc_args: {}{} + project_build_mode_defines_bm_internal__", name, suffixData.buildDefines);
			stream << std::format(",\n\tcpp_args: {}{} + project_build_mode_defines_bm_internal__", name, suffixData.buildDefines);
		}
		else
		{
			stream << std::format(",\n\tc_args: {}{} + project_build_mode_defines_bm_internal__", name, suffixData.buildDefines);
			stream << std::format(",\n\tcpp_args: {}{} + project_build_mode_defines_bm_internal__", name, suffixData.buildDefines);
		}
		stream << std::format(", \n\tlink_args: {}{}[host_machine.system()]", name, suffixData.linkArgs);
		stream << ",\n\tgnu_symbol_visibility: 'hidden'";
		stream << "\n)\n";
	}

	if(targetType != TargetType::Executable)
	{
		stream << std::format("{}_dep = declare_dependency(\n", name);
		if(targetType != TargetType::HeaderOnlyLibrary)
			stream << "\tlink_with: " << name << ",\n";
		stream << std::format("\tinclude_directories: [inc_bm_internal__, {}{}],\n", name, suffixData.includeDirs);
		stream << std::format("\tcompile_args: {}{} + project_build_mode_defines_bm_internal__\n", name, suffixData.useDefines);
		stream << ")\n";
		if(isInstall)
		{
			stream << "pkgmod.generate(";
			if(targetType != TargetType::HeaderOnlyLibrary)
				stream << name << ",\n";
			stream << "\tname: " << single_quoted_str(GetJsonKeyValue<std::string>(targetJson, "friendly_name", projMetaInfo.name)) << ",\n";
			stream << "\tdescription: " << single_quoted_str(GetJsonKeyValue<std::string>(targetJson, "description", projMetaInfo.description)) << ",\n";
			stream << "\tfilebase: " << single_quoted_str(name) << ",\n";
			stream << "\tinstall_dir: pkgconfig_install_path_bm_internal__,\n";
			if(targetType == TargetType::HeaderOnlyLibrary)
			{
				stream << "\tsubdirs: ";
				ProcessStringList(targetJson, "subdirs", stream);
				stream << ", ";
			}
			stream << std::format("\textra_cflags: {}{} + project_build_mode_defines_bm_internal__\n", name, suffixData.useDefines);
			stream << ")\n";
		}
	}
}

static void ProcessStringListDict(const json& jsonObj, const std::vector<std::pair<std::string_view, std::string_view>>& jsonKeys, std::ostringstream& stream, const std::string_view delimit = " ")
{
	for(std::size_t i = 0; const auto& key : jsonKeys)
	{
		stream << std::format("'{}' : [{}]", key.first, GetListStringOrEmpty(jsonObj, key.second));
		if(++i < jsonKeys.size())
			stream << "," << delimit;
	}
}

static void ProcessStringListDictDeclare(const json& targetJson, std::ostringstream& stream, const std::vector<std::pair<std::string_view, std::string_view>>& jsonKeys, const std::string_view suffix)
{
	std::string name = GetJsonKeyValue<std::string>(targetJson, "name");
	stream << name << suffix << " = {\n";
	ProcessStringListDict(targetJson, jsonKeys, stream, "\n");
	stream << "\n";
	stream << "}\n";
}

static void ProcessTargetJson(const json& targetJson, std::ostringstream& stream, ProjectMetaInfo& projMetaInfo)
{
	stream << "# -------------- Target: " << GetJsonKeyValue<std::string>(targetJson, "name") << " ------------------\n";
	VarSuffixData suffixData { };
	suffixData.sources = "_sources_bm_internal__";
	suffixData.dependencies = "_dependencies_bm_internal__";
	suffixData.linkArgs = "_link_args_bm_internal__";
	suffixData.includeDirs = "_include_dirs_bm_internal__";
	ProcessStringListDeclare(targetJson, stream, "sources", suffixData.sources);
	ProcessStringListDeclare(targetJson, stream, "include_dirs", suffixData.includeDirs);
	ProcessStringListDeclare(targetJson, stream, "dependencies", suffixData.dependencies, [](std::string_view quotedToken) -> std::string
	{
		return std::format("dependency({})", quotedToken);
	});
	ProcessStringListDictDeclare(targetJson, stream, 
		{ 
			{ "windows", "windows_link_args" },
			{ "linux", "linux_link_args" },
			{ "darwin", "darwin_link_args" }
		}, suffixData.linkArgs);
	TargetType targetType = DetectTargetType(targetJson);
	if(targetType == TargetType::Executable)
	{
		suffixData.buildDefines = "_defines_bm_internal__";
		ProcessStringListDeclare(targetJson, stream, "defines", suffixData.buildDefines);
		ProcessTarget(targetJson, stream, TargetType::Executable, projMetaInfo, suffixData);
	}
	// Static Library, Shared Library, and Header Only Library targets
	else
	{
		suffixData.buildDefines = "_build_defines_bm_internal__";
		suffixData.useDefines = "_use_defines_bm_internal__";
		ProcessStringListDeclare(targetJson, stream, "build_defines", suffixData.buildDefines);
		ProcessStringListDeclare(targetJson, stream, "use_defines", suffixData.useDefines);
		ProcessTarget(targetJson, stream, targetType, projMetaInfo, suffixData);
	}
}

static void SubstitutePlaceholderJson(std::string& str, const json& jsonObj, const std::string_view placeholderName, const std::string_view jsonKey)
{
	SubstitutePlaceholder(str, placeholderName, [&jsonObj, &jsonKey]()
	{
		return GetListStringOrEmpty(jsonObj, jsonKey);
	});
}

static constexpr std::pair<std::string_view, std::string_view> gPlaceHolderToJsonKeyMappings[] =
{
	{ "$$release_defines$$", "release_defines" },
	{ "$$debug_defines$$", "debug_defines" },
	{ "$$defines$$", "defines" },
	{ "$$sources$$", "sources" },
	{ "$$include_dirs$$", "include_dirs" },
	{ "$$windows_link_args$$", "windows_link_args" },
	{ "$$linux_link_args$$", "linux_link_args" },
	{ "$$darwin_link_args$$", "darwin_link_args" }
};

static std::string ProcessTemplate(std::string_view templateStr, const json& buildMasterJson)
{
	std::string str { templateStr };
	SubstitutePlaceholder(str, "$$project_name$$", single_quoted_str(GetJsonKeyValue<std::string>(buildMasterJson, "project_name")));
	SubstitutePlaceholder(str, "$$canonical_name$$", single_quoted_str(GetJsonKeyValue<std::string>(buildMasterJson, "canonical_name")));
	for(const auto& pair : gPlaceHolderToJsonKeyMappings)
		SubstitutePlaceholderJson(str, buildMasterJson, pair.first, pair.second);

	SubstitutePlaceholder(str, "$$pre_config_hook$$", [&buildMasterJson]() -> std::string
	{
		if(auto result = GetJsonKeyValueOrNull<std::string>(buildMasterJson, "pre_config_hook"); result.has_value())
		{
			constexpr std::string_view formatStr = 
R"(prehook_result = run_command(find_program('bash'), '{}', check : false)
if prehook_result.returncode() == 0
        message('Running Pre-configure hook run success')
else
        error('Pre-configure hook returned non-zero return code')
endif)";
			return std::format(formatStr, result.value());
		}
		return "";
	});
	SubstitutePlaceholder(str, "$$vars$$", [&buildMasterJson]() -> std::string
	{
		auto it = buildMasterJson.find("vars");
		if(it == buildMasterJson.end())
			return "";
		std::string str;
		 for (const auto& [key, value] : it->items())
		 {
			if(value.is_array())
			{
			    std::ostringstream stream;
			    stream << key << " = ";
			    ProcessStringList(value, stream, "\n");
			    str.append(stream.str());
			}
			else
				str.append(std::format("{} = {}\n", key, value.template get<std::string>()));
		}
		return str;
	});
	// TODO: Use ProcessStringListDeclare() instead
	SubstitutePlaceholder(str, "$$dependencies$$", [&buildMasterJson]() -> std::string
	{
		auto it = buildMasterJson.find("dependencies");
		if(it == buildMasterJson.end())
			return "";
		std::string str; 
		for(std::size_t i = 0; const auto& value : it.value())
		{
			str.append(std::format("dependency('{}')", value.template get<std::string>()));
			if(++i < it.value().size())
				str.append(",\n");
		}
		return str;
	});
	SubstitutePlaceholder(str, "$$install_subdirs$$", [&buildMasterJson]() -> std::string
	{
		auto it = buildMasterJson.find("install_header_dirs");
		if(it == buildMasterJson.end())
			return "";
		std::string str;
		for(const auto& value : it.value())
			str.append(std::format("install_subdir('{}', install_dir : get_option('includedir'))\n", value.template get<std::string>()));
		return str;
	});
	SubstitutePlaceholder(str, "$$install_headers$$", [&buildMasterJson]() -> std::string
	{
		auto it = buildMasterJson.find("install_headers");
		if(it == buildMasterJson.end())
			return "";
		std::ostringstream stream;
		for(const auto& value : it.value())
		{
			stream << "install_headers(";
			ProcessStringList(value, "files", stream, "\n");
			auto it = value.find("subdir");
			if(it != value.end())
				stream << ", subdir: " << single_quoted_str((*it).template get<std::string>());
			stream << ")\n";
		}
		return stream.str();
	});
	SubstitutePlaceholder(str, "$$build_targets$$", [&buildMasterJson]() -> std::string
	{
		ProjectMetaInfo projMetaInfo;
		projMetaInfo.name = GetJsonKeyValue<std::string>(buildMasterJson, "project_name");
		projMetaInfo.description = GetJsonKeyValue<std::string>(buildMasterJson, "description", "Description not provided");
		auto& targets = buildMasterJson["targets"];
		std::ostringstream stream;
		for(const auto& target : targets)
		{
			ProcessTargetJson(target, stream, projMetaInfo);
			stream << "\n";
		}
		return stream.str();
	});
	return str;
}

// directory: value passed to --directory flag
static void GenerateMesonBuildScript(std::string_view directory)
{
	auto mesonBuildScriptFilePath = GetMesonBuildScriptFilePath(directory);
	std::cout << std::format("Generating {}", mesonBuildScriptFilePath) << "\n";
	json buildMasterJson = ParseBuildMasterJson(directory);
	std::string templateStr = LoadMesonBuildScriptTemplate();
	std::string concreteStr = ProcessTemplate(templateStr, buildMasterJson);
	std::ofstream stream(mesonBuildScriptFilePath.data(), std::ios_base::trunc);
	if(!stream.is_open())
	{
		std::cerr << "Error: Failed to open/create " << mesonBuildScriptFilePath << "\n";
		exit(EXIT_FAILURE);
	}
	stream << std::format("#------------- Generated By Build Master {} ------------------\n\n", BUILDMASTER_VERSION_STRING);
	stream << concreteStr;
}

// directory: value passed to --directory flag
static void RegenerateMesonBuildScript(std::string_view directory = "", bool isForce = false)
{
	if(!std::filesystem::exists(GetBuildMasterJsonFilePath(directory)))
	{
		std::cerr << "Error: build_master.json doesn't exists, please execute the following:\n"
					"build_master init --name <your project name> --canonical_name <filename friendly project name>\n";
		exit(EXIT_FAILURE);
	}

	if(isForce || IsRegenerateMesonBuildScript(directory))
		GenerateMesonBuildScript(directory);
	else
		std::cout << "Info: meson.build is upto date\n";
}

// Stores values of the arguments passed to 'init' command
// Example: build_master init --name=BufferLib --canonical_name=bufferlib --force
struct ProjectInitCommandArgs
{
	// --name=<your project name>
	// NOTE: This can't be empty
	std::string_view projectName;
	// --canonical_name=<
	// NOTE: This can't be empty
	std::string_view canonicalName;
	// --directory=/path/to/a/directory
	// NOTE: This can be empty
	std::string_view directory;
	// --create-cpp
	bool isCreateCpp { false };
	// --force
	bool isForce { false };
};

// Create a new text file and writes the text data passed as argument
static void WriteTextFile(const std::string_view filePath, const std::string_view textData)
{
	std::ofstream file(filePath.data(), std::ios_base::trunc);
	if(!file.is_open())
	{
		std::cout << std::format("Error: Failed to create {} file\n", filePath);
		exit(EXIT_FAILURE);
	}
	file << textData << std::endl;
	file.close();
}

static void GenerateSeedFiles(const ProjectInitCommandArgs& args)
{
	// Generate source/main.cpp
	const std::string_view mainSourceFileName { args.isCreateCpp ? "main.cpp" : "main.c" };
	auto sourcesDirPath = GetPathStrRelativeToDir(args.directory, "source");
	if(std::filesystem::exists(sourcesDirPath))
		std::cout << std::format("Info: directory \"{}\" already exists, skipping creating \"{}\" in it\n", sourcesDirPath, mainSourceFileName);
	else
	{
		if(!std::filesystem::create_directory(sourcesDirPath))
		{
			std::cout << std::format("Error: Failed to create directory {}\n", sourcesDirPath);
			exit(EXIT_FAILURE);
		}

		auto mainSourcePath = GetPathStrRelativeToDir(sourcesDirPath, mainSourceFileName);
		const std::string_view mainSourceInitData = args.isCreateCpp ?
R"(
#include <iostream>

int main()
{
	std::cout << "Hello World";
	return 0;
}
)" :
R"(
#include <stdio.h>

int main()
{
	puts("Hello World");
	return 0;
}
)";
		WriteTextFile(mainSourcePath, mainSourceInitData);
		std::cout << std::format("Info: {} is generated\n", mainSourcePath);
	}
	// Generate include directory
	auto includeDirPath = GetPathStrRelativeToDir(args.directory, "include");
	if(std::filesystem::exists(includeDirPath))
		std::cout << std::format("Info: directory \"{}\" already exists, no need to create\n", includeDirPath);
	else
	{
		if(!std::filesystem::create_directory(includeDirPath))
		{
			std::cout << std::format("Error: Failed to create directory {}\n", includeDirPath);
			exit(EXIT_FAILURE);
		}
		std::cout << std::format("Info: Created directory \"{}\"\n", includeDirPath);
	}
}

//  build_master init
static void IntializeProject(const ProjectInitCommandArgs& args)
{
	std::cout << std::format("Project Name = \"{}\", Canonical Name = \"{}\"", args.projectName, args.canonicalName) << "\n";
	const auto filePathStr =  GetBuildMasterJsonFilePath(args.directory);
	if(!args.isForce && std::filesystem::exists(filePathStr))
	{
		std::cerr << "Error: build_master.json already exists, please remove it first or pass --force to overwrite it\n";
		exit(EXIT_FAILURE);
	}
	json buildMasterJson = 
	{
		{ "project_name" , args.projectName },
		{ "canonical_name", args.canonicalName },
		{ "include_dirs", "include" },
		{ "targets",
			{			
				{ 
					{ "name", args.canonicalName },
					{ "is_executable", true },
					{ "sources", { args.isCreateCpp ? "source/main.cpp" : "source/main.c" } }
				}
			}
		}
	};
	WriteTextFile(filePathStr, buildMasterJson.dump(4, ' ', true));
	std::cout << std::format("Success: {} is generated\n", filePathStr);

	GenerateSeedFiles(args);
}

static std::ostream& operator<<(std::ostream& stream, const std::vector<std::string>& v)
{
	for(const auto& value : v)
		stream << value << " ";
	return stream;
}

// Returns absolute path of a shell command
// It internally runs "which"
static std::optional<std::string> GetExecutablePath(std::string_view executable)
{
    std::vector<const char*> args = { gWhichCmd.data(), executable.data(), nullptr};

    reproc_t* process = reproc_new();
    reproc_options options = {};

    char buffer[4096];
    std::string result;
    bool isSuccess = true;

    auto r = reproc_start(process, args.data(), options);
    if (r >= 0)
    {
        // Read the output
        while (true)
        {
            int bytes_read = reproc_read(process, REPROC_STREAM_OUT, reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));
            if (bytes_read < 0)
                break;
            result.append(buffer, bytes_read);
        }
        reproc_wait(process, REPROC_INFINITE);
    } else isSuccess = false;
    reproc_destroy(process);
    // Remove trailing newline or carrage return characters if present
    if (!result.empty())
    	while(result.back() == '\n' || result.back() == '\r')
        	result.pop_back();
    if(isSuccess)
    	return { result };
    else return { };
}

// build_master meson
// directory: value passed to --directory flag
static void InvokeMeson(std::string_view directory, const std::vector<std::string>& args)
{
	// Ensure the meson.build script is upto date
	RegenerateMesonBuildScript(directory);
	// Build c-style argument list
  	std::vector<const char*> cArgs;
  	cArgs.reserve(args.size() + 1);
  	cArgs.push_back(gPython.data());
  	auto fullMesonExePath = GetExecutablePath(gMesonExecutableName);
  	if(!fullMesonExePath)
  	{
  		std::cerr << "Errro: Faild to get absolute path of "<< gMesonExecutableName << "\n";
  		return;
  	}
  	cArgs.push_back(fullMesonExePath->c_str());
  	for(const auto& arg : args)
  		cArgs.push_back(arg.data());
  	cArgs.push_back(nullptr);
  	std::cout << "Running " << gMesonExecutableName << " with args: " << args << "\n";
  	reproc_options opts { };
  	if(directory.size())
  		opts.working_directory = directory.data();
  	// Execute the meson command with the built arguments
  	auto returnCode = reproc_run(cArgs.data(), opts);
  	if(returnCode < 0)
  		std::cerr << "Error: Failed to run " << gMesonExecutableName << ", have you executed ./install_meson.sh ever?\n";
  	exit(returnCode);
}

static void PrintVersionInfo() noexcept
{
	std::cout << "Build Master " << BUILDMASTER_VERSION_STRING << "\n";
	#ifdef BUILD_MASTER_RELEASE
		std::cout << "Build Type: Release\n";
	#else
		std::cout << "Build Type: Debug\n";
	#endif
}

int main(int argc, const char* argv[])
{
	CLI::App app;

	bool isPrintVersion = false, isUpdateMesonBuild = false, isForce = false;
	std::string directory;
	app.add_flag("--version", isPrintVersion, "Prints version number of Build Master");
	app.add_flag("--update-meson-build", isUpdateMesonBuild, "Regenerates the meson.build script if the build_master.json file is more recent");
	app.add_flag("--force", isForce, "if --update-meson-build flag is present along with this --force then meson.build script is generated even if it is upto date");
	app.add_option("--directory", directory, "Directory path in which to look for build_master.json, by default it is the current working directory");

	// Project Initialization Sub command	
	{
		CLI::App* scInit = app.add_subcommand("init", "Project initialization, creates build_master.json file");
		ProjectInitCommandArgs args;
		scInit->add_option("--name", args.projectName, "Name of the Project")->required();
		scInit->add_option("--canonical_name", args.canonicalName, 
			"Conanical name of the Project, typically used for naming files")->required();
		scInit->add_flag("--force", args.isForce, "Forcefully overwrites the existing build_master.json file, use it carefully");
		scInit->add_flag("--create-cpp", args.isCreateCpp, "Create source/main.cpp instead of source/main.c, use this flag if you intend to write C++ code in start");
		scInit->add_option("--directory", args.directory, "Directory path in which build_master.json and source/main.cpp would be created, by default it is the current working directory");
		scInit->callback([&]()
		{
			// If --directory option for this `init` command is not supplied while it is supplied for build_master
			// then use the build_master's --directory option's value.
			// We considering the following case here:
			//	build_master --directory init --name=myProject --canonical_name=myproject
			if(args.directory.size() == 0)
				args.directory = directory; 
			IntializeProject(args); 
		});
	}
	
	// Meson Sub command
	{
		CLI::App* scMeson = app.add_subcommand("meson", "Invokes meson build system (cli), all the arguments passed to this subcommands goes to actual meson command");
		scMeson->allow_extras();
		scMeson->callback([&]() { InvokeMeson(directory, scMeson->remaining()); });
	}

	CLI11_PARSE(app, argc, argv);
	
	if(isPrintVersion)
	{
		PrintVersionInfo();
		return EXIT_SUCCESS;
	}
	if(isUpdateMesonBuild)
	{
		RegenerateMesonBuildScript(directory, isForce);
		return EXIT_SUCCESS;
	}

	return EXIT_SUCCESS;
}