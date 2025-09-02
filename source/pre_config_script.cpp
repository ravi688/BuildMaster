#include <build_master/pre_config_script.hpp>
#include <build_master/json_parse.hpp> // for ParseBuildMasterJson(), and GetJsonKeyValueOrNull<>()
#include <build_master/misc.hpp> // for SelectPath()

#include <spdlog/spdlog.h>
#include <invoke/invoke.hpp>
#include <cstdlib>

static constexpr std::string_view gBash = "bash";

static std::optional<bool> RunPreConfigScript(const json& buildMasterJson, std::string_view directory, std::string_view hookName, std::string_view logMsg, bool isRoot = false)
{
	if(auto result = GetJsonKeyValueOrNull<std::string>(buildMasterJson, hookName); result.has_value())
	{
		spdlog::info(logMsg);
		std::optional<std::vector<std::string>> bashPaths = invoke::GetExecutablePaths(gBash);
		if(!bashPaths)
		{
			spdlog::error("No path found for {}", gBash);
			exit(EXIT_FAILURE);
		}
		std::string bashPath = SelectPath(bashPaths.value()); 
		auto returnCode = invoke::Exec({ bashPath, result.value() }, directory, isRoot);
		return { returnCode == 0 };
	}
	return { };
}

bool RunPreConfigScript(std::string_view directory)
{
	json buildMasterJson = ParseBuildMasterJson(directory);

	// Run pre_config_hook	
	auto result1 = RunPreConfigScript(buildMasterJson, directory, "pre_config_hook", "Running pre-config hook script");
	if(result1)
	{
		if(result1.value())
			spdlog::info("pre-config hook script run success");
		else
			spdlog::info("pre-config hook script returned non-zero code");
	}

	// Run pre_config_root_hook
	auto result2 = RunPreConfigScript(buildMasterJson, directory, "pre_config_root_hook", "Running pre-config hook script with root privileges", true);
	if(result2)
	{
		if(result2.value())
			spdlog::info("pre-config hook script with root privileges run success");
		else
			spdlog::info("pre-config hook script with root privileges returned non-zero code");
	}

	return result1 || result2;
}
