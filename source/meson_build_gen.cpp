#include <build_master/misc.hpp> // for LoadTextFile(), and GetPathStrRelativeToDir()
#include <build_master/json_parse.hpp> // for ParseBuildMasterJson(), and GetJsonKeyValue<>()
#include <build_master/version.hpp>

#include <iostream>
#include <cstdlib>
#include <format>
#include <iterator>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>
#include <fstream>

#include <spdlog/spdlog.h>

static constexpr std::string_view gMesonBuildScriptFilePath = "meson.build";

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

static void ProcessStringListElements(const json& jsonObj, std::ostringstream& stream, std::string_view delimit = " ", std::optional<TokenTransformCallback> callback = { }, bool isMetaInfo = true)
{
	for(std::size_t i = 0; const auto& value : jsonObj)
	{
		const auto& str = value.template get<std::string>();
		std::string token = isMetaInfo ? ApplyMetaInfo(str) : str;
		if(callback)
			token = (*callback)(token);
		stream << token;
		if(++i < jsonObj.size())
			stream << "," << delimit;
	}
}

static void ProcessStringListElements(const json& jsonObj, std::string_view keyName, std::ostringstream& stream, std::string_view delimit = " ", std::optional<TokenTransformCallback> callback = { }, bool isMetaInfo = true)
{
	auto it = jsonObj.find(keyName);
	if(it == jsonObj.end())
		return;
	ProcessStringListElements(it.value(), stream, delimit, callback, isMetaInfo);
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

static void ProcessStringList(const json& jsonObj, std::ostringstream& stream, std::string_view delimit = " ", std::optional<TokenTransformCallback> callback = { }, bool isMetaInfo = true)
{
	stream << "[\n";
	ProcessStringListElements(jsonObj, stream, delimit, callback, isMetaInfo);
	stream << "\n";
	stream << "]\n";
}

static void ProcessStringList(const json& jsonObj, std::string_view keyName, std::ostringstream& stream, std::string_view delimit = " ", std::optional<TokenTransformCallback> callback = { }, bool isMetaInfo = true)
{
	stream << "[\n";
	ProcessStringListElements(jsonObj, keyName, stream, delimit, callback, isMetaInfo);
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
	std::string_view includeDirs;
	std::string_view platformSpecificSources;
	std::string_view buildDefines;
	std::string_view useDefines;
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
		stream << std::format(",\n\t{}{} + {}{}[host_machine.system()] + sources_bm_internal__", name, suffixData.sources, name, suffixData.platformSpecificSources);
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
		if(auto listJson = GetJsonKeyValueOrNull<json>(targetJson, "link_with"))
		{
			stream << ", \n\tlink_with: ";
			ProcessStringList(listJson.value(), stream, " ", { }, false);
		}
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
	suffixData.platformSpecificSources = "_platform_src_bm_internal__";
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
	ProcessStringListDictDeclare(targetJson, stream,
		{
			{ "windows", "windows_sources" },
			{ "linux", "linux_sources" },
			{ "darwin", "darwin_sources" }
		}, suffixData.platformSpecificSources);
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
	{ "$$windows_sources$$", "windows_sources" },
	{ "$$linux_sources$$", "linux_sources" },
	{ "$$darwin_sources$$", "darwin_sources" },
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
void RegenerateMesonBuildScript(std::string_view directory, bool isForce)
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
