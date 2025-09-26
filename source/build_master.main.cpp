#include <build_master/meson_build_gen.hpp> // for RegenerateMesonBuildScript()
#include <build_master/invoke_meson.hpp> // for InvokeMeson()
#include <build_master/pre_config_script.hpp> // for RunPreConfigScript()
#include <build_master/misc.hpp> // for GetBuildMasterJsonFilePath()
#include <build_master/json_parse.hpp>
#include <build_master/version.hpp>

#include <iostream>
#include <cstdlib>
#include <format>
#include <iterator>
#include <filesystem>
#include <string>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <common/defines.hpp> // for com::to_upper()
#include <invoke/root.hpp> // for root privileges utility functions in invoke namespace

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

// Creates a text file with initData if doesn't exist already. It also creates any intermediate directories if doesn't exist.
static void CreateFile(const std::filesystem::path& path, std::string_view initData)
{
	auto pathCopy = path;
	auto dirs = pathCopy.remove_filename();
	try
	{
		std::filesystem::create_directories(dirs);
	} catch(const std::exception& except)
	{
		std::cerr << "Error: Failed to create " << dirs.string() << ", " << except.what();
	}
	if(std::filesystem::exists(path))
		std::cout << std::format("Info: file \"{}\" already exists, skipping creating\n", path.string());
	else
		WriteTextFile(path.string(), initData);
}

using namespace std::literals;

// include/<canonical_name>/api_defines.h or .hpp
// include/<canonical_name>/defines.h or .hpp
// source/main.cpp
static void GenerateSeedFiles(const ProjectInitCommandArgs& args)
{
	constexpr std::string_view mainSourceInitData[2] = {
R"(
#include <iostream>

int main()
{
	std::cout << "Hello World";
	return 0;
}
)",
R"(
#include <stdio.h>

int main()
{
	puts("Hello World");
	return 0;
}
)" };
	constexpr std::string_view apiDefinesInitTemplate =
R"(
#pragma once

#if (defined _WIN32 || defined __CYGWIN__) && defined(__GNUC__)
#	define {0}_IMPORT_API __declspec(dllimport)
#	define {0}_EXPORT_API __declspec(dllexport)
#else
#	define {0}_IMPORT_API __attribute__((visibility("default")))
#	define {0}_EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef {0}_BUILD_STATIC_LIBRARY
#	define {0}_API
#elif defined({0}_BUILD_DYNAMIC_LIBRARY)
#	define {0}_API {0}_EXPORT_API
#elif defined({0}_USE_DYNAMIC_LIBRARY)
#	define {0}_API {0}_IMPORT_API
#elif defined({0}_USE_STATIC_LIBRARY)
#	define {0}_API
#else
#	define {0}_API
#endif
)";
	constexpr std::string_view definesInitTemplate =
R"(
#pragma once
#include <{0}/api_defines.{1}>
#if !defined({2}_RELEASE) && !defined({2}_DEBUG)
#   warning "None of {2}_RELEASE && {2}_DEBUG is defined; using {2}_DEBUG"
#   define {2}_DEBUG
#endif
)";
	auto canonicalNameUpper = com::to_upper(args.canonicalName);
	std::string apiDefinesInitData = std::format(apiDefinesInitTemplate, canonicalNameUpper);
	std::string_view headerFileExt { args.isCreateCpp ? "hpp" : "h" };
	std::string definesInitData = std::format(definesInitTemplate, args.canonicalName, headerFileExt, canonicalNameUpper);
	auto sourceDir = std::filesystem::path(args.directory) / "source"sv;
	auto includeDir = std::filesystem::path(args.directory) / "include"sv / args.canonicalName;
	if(args.isCreateCpp)
	{
		CreateFile(sourceDir / "main.cpp", mainSourceInitData[0]);
		CreateFile(includeDir / "api_defines.hpp", apiDefinesInitData);
		CreateFile(includeDir / "defines.hpp", definesInitData);
	}
	else
	{
		CreateFile(sourceDir / "main.c", mainSourceInitData[1]);
		CreateFile(includeDir / "api_defines.h", apiDefinesInitData);
		CreateFile(includeDir / "defines.h", definesInitData);
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

static void PrintVersionInfo() noexcept
{
	std::cout << "Build Master " << BUILDMASTER_VERSION_STRING << "\n";
	#ifdef BUILD_MASTER_RELEASE
		std::cout << "Build Type: Release\n";
	#else
		std::cout << "Build Type: Debug\n";
	#endif
	std::cout << "Git Commit ID: " << GIT_COMMIT_ID << "\n";
}

int main(int argc, const char* argv[])
{
#ifdef PLATFORM_LINUX
	// Drop root privileges if the build_master command is executed under root privileges, root privileges need to be used where absolutely necessary,
	// And any modifications by root should be kept at minimum.
	if(invoke::HasRootPrivileges())
	{
		spdlog::info("Running as root");
		if(invoke::DropRootPrivileges())
			spdlog::warn("Dropped root privileges");
	}
#endif // PLATFORM_LINUX

	CLI::App app;

	bool isPrintVersion = false, 
		isUpdateMesonBuild = false,
		isMesonBuildTemplatePath = false,
		isExecutePreConfigHook = false,
		isForce = false;
	std::string directory;
	app.add_flag("--version", isPrintVersion, "Prints version number of Build Master");
	app.add_flag("--update-meson-build", isUpdateMesonBuild, "Regenerates the meson.build script if the build_master.json file is more recent");
	app.add_flag("--meson-build-template-path", isMesonBuildTemplatePath, "Prints lookup path of meson.build.template, typically this is only for debugging purpose");
	app.add_flag("--execute-pre-config-hook", isExecutePreConfigHook, "Executes pre_config_hook (shell script) if any");
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
	if(isMesonBuildTemplatePath)
	{
		spdlog::info("MESON_BUILD_TEMPLATE_PATH: {}", MESON_BUILD_TEMPLATE_PATH);
		return EXIT_SUCCESS;
	}
	if(isExecutePreConfigHook)
	{
		if(!RunPreConfigScript(directory))
			spdlog::info("No pre_config_hook to run");
	}

	return EXIT_SUCCESS;
}
