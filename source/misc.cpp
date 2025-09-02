#include <build_master/misc.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

#include <spdlog/spdlog.h>

std::string LoadTextFile(std::string_view filePath)
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

// directoryBase: The base directory against which the the final path need to be calculated
// relativeDir: The path relative to the base directory passed as 'directoryBase' (first arg)
std::string GetPathStrRelativeToDir(std::string_view directoryBase, std::string_view relativePath)
{
	auto buildMasterJsonFilePath = std::filesystem::path(directoryBase) / std::filesystem::path(relativePath);
	return buildMasterJsonFilePath.string();
}

static constexpr std::string_view gBuildMasterJsonFilePath = "build_master.json";

// Use this function whenever you meant to get gBuildMasterJsonFilePath.
// DO NOT use gBuildMasterJsonFilePath directly as that would not consider the --directory flag 
std::string GetBuildMasterJsonFilePath(std::string_view directory)
{
	return GetPathStrRelativeToDir(directory, gBuildMasterJsonFilePath);
}

std::string SelectPath(const std::vector<std::string>& paths)
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
