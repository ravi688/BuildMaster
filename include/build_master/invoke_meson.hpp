#pragma once

#include <string_view>
#include <vector>
#include <string>


void InvokeMeson(std::string_view directory, const std::vector<std::string>& args, bool isBuildMasterJsonAvailable = true);
