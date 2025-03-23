#include <build_master/meson_build_gen.hpp>
#include <build_master/pre_config_script.hpp>

#include <iostream>
#include <cstdlib>
#include <format>
#include <iterator>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>
#include <sstream>

#include <spdlog/spdlog.h>
#include <invoke/invoke.hpp>

static constexpr std::string_view gMesonExecutableName = "build_master_meson";

static std::ostream& operator<<(std::ostream& stream, const std::vector<std::string>& v)
{
	for(const auto& value : v)
		stream << value << " ";
	return stream;
}

static std::string SelectPath(const std::vector<std::string>& paths)
{
	#ifdef _WIN32
		// First try mingw
		for(const auto& path : paths)
			if(path.find("mingw") != std::string::npos)
				return path;
		// Then fallback to msys
		for(const auto& path : paths)
			if(path.find("msys") != std::string::npos)
				return path;
		spdlog::error("Couldn't find mingw or msys equivalent executable path");
		exit(EXIT_FAILURE);
	#else
		return paths[0];
	#endif
}

// cmdName: name of the command (not the full path, just the name)
// restArgs: The rest of the arguments which need to be passed to that cmd
static std::vector<std::string> BuildArgumentsForCmdRun(std::string_view cmdName, const std::vector<std::string>& restArgs)
{
  	std::vector<std::string> finalArgs;
  	finalArgs.reserve(restArgs.size() + 1);
  	auto paths = invoke::GetExecutablePaths(cmdName);
  	if(!paths)
  	{
  		spdlog::error("Couldn't find paths for the executable: {}", cmdName);
  		exit(EXIT_FAILURE);
  	}
  	auto fullMesonExePath = SelectPath(paths.value());;
  	finalArgs.push_back(fullMesonExePath);
  	for(const auto& arg : restArgs)
  		finalArgs.push_back(arg);
  	return finalArgs;
}

// cmdName: Just the name of the command
// restArgs: The rest of the argumentst which need to be passed to that cmd
static decltype(auto) RunCmd(std::string_view cmdName, std::string_view workDirectory, const std::vector<std::string>& restArgs)
{
  	// Prepare arguments
  	std::vector<std::string> finalArgs = BuildArgumentsForCmdRun(cmdName, restArgs);
  	// Logging
  	std::stringstream strstream;
  	strstream << "Command: " << finalArgs << "\n";
  	spdlog::info(strstream.str());
  	return invoke::Exec(finalArgs, workDirectory);	
}

// build_master meson
// directory: value passed to --directory flag
void InvokeMeson(std::string_view directory, const std::vector<std::string>& args, bool isBuildMasterJsonAvailable)
{
	if(isBuildMasterJsonAvailable)
	{
		// Ensure the meson.build script is upto date
		RegenerateMesonBuildScript(directory);
		// Run pre-configure script if 'meson setup' command is executed
		if(args.size() == 0 || args[0] == "setup")
			RunPreConfigScript(directory);
	}
	// Run the meson command
  	exit(RunCmd(gMesonExecutableName, directory, args));
}
