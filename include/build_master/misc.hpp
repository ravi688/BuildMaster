#pragma once

#include <string>
#include <string_view>

std::string LoadTextFile(std::string_view filePath);
std::string GetPathStrRelativeToDir(std::string_view directoryBase, std::string_view relativePath);
std::string GetBuildMasterJsonFilePath(std::string_view directory);
