#pragma once

#include <string>
#include <string_view>
#include <vector>

std::string LoadTextFile(std::string_view filePath);
std::string GetPathStrRelativeToDir(std::string_view directoryBase, std::string_view relativePath);
std::string GetBuildMasterJsonFilePath(std::string_view directory);

// Selects a single path out of multiple given paths as follows:
// 1. If the compilation platform is Windows then it chooses paths containing mingw, if not found then it looks for msys
// 2. If the compilation platform is other than Windows then it chooses the first path at index 0.
std::string SelectPath(const std::vector<std::string>& paths);
