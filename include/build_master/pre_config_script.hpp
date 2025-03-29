#pragma once

#include <string_view>

// Returns true if either of 'pre_config_hook' or 'pre_config_root_hook' is given
bool RunPreConfigScript(std::string_view directory);
