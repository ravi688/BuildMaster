#include <iostream>
#include <cstdlib>
#include <format>
#include <iterator>
#include <filesystem>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

//  build_master init
static void intializeProject(std::string_view projectName, std::string_view canonicalName, bool isForce)
{
	std::cout << std::format("Project Name = \"{}\", Canonical Name = \"{}\"", projectName, canonicalName) << "\n";
	if(!isForce && std::filesystem::exists("build_master.json"))
	{
		std::cout << "Error: build_master.json already exists, please remove it first or pass --force to overwrite it\n";
		exit(EXIT_FAILURE);
	}
	json buildMasterJson = 
	{
		{ "header_dir", "include" },
		{ "canonical_name", canonicalName },
		{ "project_name" , projectName },
		{ "targets",
			{ canonicalName, 
				{
					{ "is_executable", true },
					{ "sources", { "source/main.c" } }
				}
			}
		}
	};

	std::ofstream file("build_master.json", std::ios_base::trunc);
	if(!file.is_open())
	{
		std::cout << "Error: Failed to create build_master.json file\n";
		exit(EXIT_FAILURE);
	}
	file << buildMasterJson.dump(4, ' ', true) << std::endl;
	file.close();
	std::cout << "Success: build_master.json is generated\n";
}

// build_master meson
static void invokeMeson(const std::vector<std::string>& args)
{
	std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, ", "));
}

static void PrintVersionInfo() noexcept
{
	std::cout << "Build Master 1.0.0\n";
	#ifdef BUILD_MASTER_RELEASE
		std::cout << "Build Type: Release\n";
	#else
		std::cout << "Build Type: Debug\n";
	#endif
}

int main(int argc, const char* argv[])
{
	CLI::App app;

	bool isPrintVersion;
	app.add_flag("--version", isPrintVersion, "Prints version number of Build Master");


	// Project Initialization Sub command	
	{
		CLI::App* scInit = app.add_subcommand("init", "Project initialization, creates build_master.json file");
		std::string projectName;
		std::string canonicalName;
		bool isForce;
		scInit->add_option("--name", projectName, "Name of the Project")->required();
		scInit->add_option("--canonical_name", canonicalName, 
			"Conanical name of the Project, typically used for naming files")->required();
		scInit->add_flag("--force", isForce, "Forcefully overwrites the existing build_master.json file, use it carefully");
		scInit->callback([&]() { intializeProject(projectName, canonicalName, isForce); });
	}
	
	// Meson Sub command
	{
		CLI::App* scMeson = app.add_subcommand("meson", "Invokes meson build system (cli), all the arguments passed to this subcommands goes to actual meson command");
		scMeson->allow_extras();
		scMeson->callback([&]() { invokeMeson(scMeson->remaining()); });
	}

	CLI11_PARSE(app, argc, argv);
	
	if(isPrintVersion)
	{
		PrintVersionInfo();
		return EXIT_SUCCESS;
	}

	return EXIT_SUCCESS;
}