#include <build_master/pre_config_script.hpp>
#include <build_master/json_parse.hpp> // for ParseBuildMasterJson(), and GetJsonKeyValueOrNull<>()

#include <spdlog/spdlog.h>
#include <invoke/invoke.hpp>
#include <cstdlib>

static constexpr std::string_view gBash = "bash";

bool RunPreConfigScript(std::string_view directory)
{
	json buildMasterJson = ParseBuildMasterJson(directory);
	if(auto result = GetJsonKeyValueOrNull<std::string>(buildMasterJson, "pre_config_hook"); result.has_value())
	{
		spdlog::info("Running pre-config hook script");
		auto returnCode = invoke::Exec({ std::string { gBash }, result.value() }, directory);
		if(returnCode < 0)
		{			
			spdlog::error("pre-config hook script returned non-zero code");
			exit(returnCode);
		}
		spdlog::info("pre-config hook script run success");
		return true;
	}
	return false;
}
