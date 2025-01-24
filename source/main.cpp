#include <iostream>
#include <cstdlib>
#include <format>
#include <iterator>
#include <filesystem>
#include <string>
#include <string_view>

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

using json = nlohmann::json;

static constexpr std::string_view gBuildMasterJsonFilePath = "build_master.json";
static constexpr std::string_view gMesonBuildScriptFilePath = "meson.build";

static bool IsRegenerateMesonBuildScript()
{
	if(std::filesystem::exists(gMesonBuildScriptFilePath))
	{
		auto mesonBuildScriptTime = std::filesystem::last_write_time(gMesonBuildScriptFilePath);
		auto buildMasterJsonTime = std::filesystem::last_write_time(gBuildMasterJsonFilePath);
		return buildMasterJsonTime >= mesonBuildScriptTime;
	}
 	return true;
}

static std::string LoadTextFile(std::string_view filePath)
{
	std::ifstream stream(filePath.data());
	if(!stream.is_open())
	{
		std::cout << "Error: Failed to open " << filePath << "\n";
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

static json ParseBuildMasterJson()
{
	std::string jsonStr = LoadTextFile(gBuildMasterJsonFilePath);
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

static std::string BuildListString(const json& jsonStringList, std::string_view delimit = " ")
{
	std::string str;
	for(std::size_t i = 0; const auto& value : jsonStringList)
	{
		str.append(std::format("'{}'", value.template get<std::string>()));
		if(++i < jsonStringList.size())
		{
			str.append(",");
			str.append(delimit);
		}
	}
	return str;
}

static std::string GetListStringOrEmpty(const json& jsonObj, std::string_view keyName, std::string_view delimit = " ")
{
	auto it = jsonObj.find(keyName);
	if(it == jsonObj.end())
		return "";
	return BuildListString(it.value(), delimit);
}

static void SubstitutePlaceholder(std::string& str, std::string_view placeholderName, std::string_view substitute)
{
	auto it = str.find(placeholderName);
	if(it != std::string::npos)
		str.replace(it, placeholderName.size(), substitute);
}

static void SubstitutePlaceholder(std::string& str, std::string_view placeholderName, std::function<std::string(void)> callback)
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
	return TargetType::Executable;
}

static void ProcessStringList(const json& jsonObj, std::ostringstream& stream, std::string_view delimit = " ")
{
	for(std::size_t i = 0; const auto& value : jsonObj)
	{
		stream << std::format("'{}'", value.template get<std::string>());
		if(++i < jsonObj.size())
			stream << "," << delimit;
	}
}

static void ProcessStringList(const json& jsonObj, std::string_view keyName, std::ostringstream& stream, std::string_view delimit = " ")
{
	auto it = jsonObj.find(keyName);
	if(it == jsonObj.end())
		return;
	ProcessStringList(it.value(), stream, delimit);
}

template<typename T>
static T GetJsonKeyValue(const json& jsonObj, std::string_view key, std::optional<T> defaultValue = {})
{
	if(auto it = jsonObj.find(key); it != jsonObj.end())
		return it.value().template get<T>();
	if(!defaultValue.has_value())
		throw std::runtime_error(std::format("No such key: {} exists in the json provided, and default value is not given either", key));
	return std::move(defaultValue.value());
}

static void ProcessStringListDeclare(const json& targetJson, std::ostringstream& stream, std::string_view listName, std::string_view suffix)
{
	std::string name = GetJsonKeyValue<std::string>(targetJson, "name");
	stream << name << suffix << " = [\n";
	ProcessStringList(targetJson, listName, stream, "\n");
	stream << "\n";
	stream << "]\n";
}

static constexpr std::string_view GetTargetTypeStr(TargetType targetType)
{
	switch(targetType)
	{
		case TargetType::StaticLibrary : return "static_library";
		case TargetType::SharedLibrary : return "shared_library";
		case TargetType::Executable : return "executable";
	}
	return "executable";
}

struct ProjectMetaInfo
{
	std::string name;
	std::string description;
};

static std::string single_quoted_str(std::string_view str)
{
	return std::format("'{}'", str);
}

static void ProcessTarget(const json& targetJson, std::ostringstream& stream,
				TargetType targetType,
				ProjectMetaInfo& projMetaInfo,
				std::string_view sources_suffix = "", 
				std::string_view link_args_suffix = "",
				std::string_view build_defines_suffix = "", 
				std::string_view use_defines_suffix = "")
{
	std::string name = GetJsonKeyValue<std::string>(targetJson, "name");
	// Libraries have is_install set to true by default
	// Executables have is_install set to false by default
	bool isInstall = GetJsonKeyValue<bool>(targetJson, "is_install", (targetType == TargetType::Executable) ? false : true);
	std::string_view targetTypeStr = GetTargetTypeStr(targetType);
	stream << std::format("{} = {}('{}'", name, targetTypeStr, name);
	stream << std::format(",\n\t{}{} + sources", name, sources_suffix);
	stream << std::format(",\n\tdependencies: dependencies");
	stream << std::format(",\n\tinclude_directories: inc");
	stream << std::format(",\n\tinstall: {}", isInstall ? "true" : "false");
	if(targetType != TargetType::Executable)
	{
		stream << ",\n\tinstall_dir: lib_install_dir";
		stream << std::format(",\n\tc_args: {}{} + {}{}", name, build_defines_suffix, name, use_defines_suffix);
		stream << std::format(",\n\tcpp_args: {}{} + {}{}", name, build_defines_suffix, name, use_defines_suffix);
	}
	else
	{
		stream << std::format(",\n\tc_args: {}{}", name, build_defines_suffix);
		stream << std::format(",\n\tcpp_args: {}{}", name, build_defines_suffix);
	}
	stream << std::format(", \n\tlink_args: {}{}[host_machine.system()]", name, link_args_suffix);
	stream << ",\n\tgnu_symbol_visibility: 'hidden'";
	stream << "\n)\n";

	if(targetType != TargetType::Executable)
	{
		stream << std::format("{} = declare_dependency(\n", name);
		stream << "\tlink_with: " << name << ",\n";
		stream << "\tinclude_directories: inc,\n";
		stream << std::format("\tcompile_args: {}{} + build_mode_defines\n", name, use_defines_suffix);
		stream << ")\n";
		if(isInstall)
		{
			stream << "pkgmod.generate(" << single_quoted_str(name) << ",\n";
			stream << "\tname: " << single_quoted_str(GetJsonKeyValue<std::string>(targetJson, "friendly_name", projMetaInfo.name)) << ",\n";
			stream << "\tdescription: " << single_quoted_str(GetJsonKeyValue<std::string>(targetJson, "description", projMetaInfo.description)) << ",\n";
			stream << "\tfilebase: " << single_quoted_str(name) << ",\n";
			stream << "\tinstall_dir: pkgconfig_install_path,\n";
			stream << std::format("\textra_cflags: {}{} + build_mode_defines\n", name, use_defines_suffix);
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
	ProcessStringListDeclare(targetJson, stream, "sources", "_sources");
	ProcessStringListDictDeclare(targetJson, stream, 
		{ 
			{ "windows", "windows_link_args" },
			{ "linux", "linux_link_args" },
			{ "darwin", "darwin_link_args" }
		}, "_link_args");
	TargetType targetType = DetectTargetType(targetJson);
	switch(targetType)
	{
		case TargetType::StaticLibrary:
		{
			ProcessStringListDeclare(targetJson, stream, "build_defines", "_build_defines");
			ProcessStringListDeclare(targetJson, stream, "use_defines", "_use_defines");
			ProcessTarget(targetJson, stream, TargetType::StaticLibrary, projMetaInfo, "_sources", "_link_args", "_build_defines", "_use_defines");
			break;
		}
		case TargetType::SharedLibrary:
		{
			ProcessStringListDeclare(targetJson, stream, "build_defines", "_build_defines");
			ProcessStringListDeclare(targetJson, stream, "use_defines", "_use_defines");
			ProcessTarget(targetJson, stream, TargetType::SharedLibrary, projMetaInfo, "_sources", "_link_args", "_build_defines", "_use_defines");
			break;
		}
		case TargetType::Executable:
		{
			ProcessStringListDeclare(targetJson, stream, "defines", "_defines");
			ProcessTarget(targetJson, stream, TargetType::Executable, projMetaInfo, "_sources", "_link_args", "_defines");
			break;
		}
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
	{ "$$sources$$", "sources" },
	{ "$$include_dirs$$", "include_dirs" },
	{ "$$windows_link_args$$", "windows_link_args" },
	{ "$$linux_link_args$$", "linux_link_args" },
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
			str.append(std::format("install_subdir('{}')\n", value.template get<std::string>()));
		return str;
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

static void GenerateMesonBuildScript()
{
	std::cout << std::format("Generating {}", gMesonBuildScriptFilePath) << "\n";
	json buildMasterJson = ParseBuildMasterJson();
	std::string templateStr = LoadMesonBuildScriptTemplate();
	std::string concreteStr = ProcessTemplate(templateStr, buildMasterJson);
	std::ofstream stream(gMesonBuildScriptFilePath.data(), std::ios_base::trunc);
	if(!stream.is_open())
	{
		std::cout << "Error: Failed to open/create " << gMesonBuildScriptFilePath << "\n";
		exit(EXIT_FAILURE);
	}
	stream << std::format("#------------- Generated By Build Master {} ------------------\n\n", BUILDMASTER_VERSION_STRING);
	stream << concreteStr;
}

static void RegenerateMesonBuildScript(bool isForce = false)
{
	if(!std::filesystem::exists(gBuildMasterJsonFilePath))
	{
		std::cout << "Error: build_master.json doesn't exists, please execute the following:\n"
					"build_master init --name <your project name> --canonical_name <filename friendly project name>\n";
		exit(EXIT_FAILURE);
	}

	if(isForce || IsRegenerateMesonBuildScript())
		GenerateMesonBuildScript();
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

//  build_master init
static void IntializeProject(const ProjectInitCommandArgs& args)
{
	std::cout << std::format("Project Name = \"{}\", Canonical Name = \"{}\"", args.projectName, args.canonicalName) << "\n";
	auto buildMasterJsonFilePath = std::filesystem::path(args.directory) / std::filesystem::path(gBuildMasterJsonFilePath);
	const auto& filePathStr = buildMasterJsonFilePath.native();
	if(!args.isForce && std::filesystem::exists(filePathStr))
	{
		std::cout << "Error: build_master.json already exists, please remove it first or pass --force to overwrite it\n";
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
	std::cout << "Success: build_master.json is generated\n";

	const std::string_view mainSourceFileName { args.isCreateCpp ? "main.cpp" : "main.c" };
	auto sourcesDirPath = std::filesystem::path(args.directory) / std::filesystem::path { "source" };
	if(std::filesystem::exists(sourcesDirPath))
		std::cout << std::format("Info: directory \"{}\" already exists, skipping creating \"{}\" in it\n", sourcesDirPath.native(), mainSourceFileName);
	else
	{
		if(!std::filesystem::create_directory(sourcesDirPath))
		{
			std::cout << std::format("Error: Failed to create directory {}\n", sourcesDirPath.native());
			exit(EXIT_FAILURE);
		}

		auto mainSourcePath = sourcesDirPath / std::filesystem::path { mainSourceFileName };
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
		WriteTextFile(mainSourcePath.native(), mainSourceInitData);
		std::cout << std::format("Info: {} is generated\n", mainSourcePath.native());
	}
}

static std::ostream& operator<<(std::ostream& stream, const std::vector<std::string>& v)
{
	for(const auto& value : v)
		stream << value << " ";
	return stream;
}

// build_master meson
static void InvokeMeson(const std::vector<std::string>& args)
{
	// Ensure the meson.build script is upto date
	RegenerateMesonBuildScript();
	// Build c-style argument list
  	std::vector<const char*> cArgs;
  	cArgs.reserve(args.size() + 1);
  	cArgs.push_back("meson");
  	for(const auto& arg : args)
  		cArgs.push_back(arg.data());
  	std::cout << "Running meson with args: " << args << "\n";
  	// Execute the meson command with the built arguments
  	exit(reproc_run(cArgs.data(), { }));
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
	app.add_flag("--version", isPrintVersion, "Prints version number of Build Master");
	app.add_flag("--update-meson-build", isUpdateMesonBuild, "Regenerates the meson.build script if the build_master.json file is more recent");
	app.add_flag("--force", isForce, "if --update-meson-build flag is present along with this --force then meson.build script is generated even if it is upto date");

	// Project Initialization Sub command	
	{
		CLI::App* scInit = app.add_subcommand("init", "Project initialization, creates build_master.json file");
		ProjectInitCommandArgs args;
		scInit->add_option("--name", args.projectName, "Name of the Project")->required();
		scInit->add_option("--canonical_name", args.canonicalName, 
			"Conanical name of the Project, typically used for naming files")->required();
		scInit->add_flag("--force", args.isForce, "Forcefully overwrites the existing build_master.json file, use it carefully");
		scInit->add_flag("--create-cpp", args.isCreateCpp, "Create source/main.cpp instead of source/main.c, use this flag if you intend to write C++ code in start");
		scInit->add_option("--directory", args.directory, "Directory path in which build_master.json would be created, by default it is the current working directory");
		scInit->callback([&]() { IntializeProject(args); });
	}
	
	// Meson Sub command
	{
		CLI::App* scMeson = app.add_subcommand("meson", "Invokes meson build system (cli), all the arguments passed to this subcommands goes to actual meson command");
		scMeson->allow_extras();
		scMeson->callback([&]() { InvokeMeson(scMeson->remaining()); });
	}

	CLI11_PARSE(app, argc, argv);
	
	if(isPrintVersion)
	{
		PrintVersionInfo();
		return EXIT_SUCCESS;
	}
	if(isUpdateMesonBuild)
	{
		RegenerateMesonBuildScript(isForce);
		return EXIT_SUCCESS;
	}

	return EXIT_SUCCESS;
}