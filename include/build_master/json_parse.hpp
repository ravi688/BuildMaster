#pragma once

#include <nlohmann/json.hpp>

#include <string_view>
#include <optional>
#include <stdexcept>
#include <format>

using json = nlohmann::ordered_json;

json ParseBuildMasterJson(std::string_view directory);

template<typename T>
std::optional<T> GetJsonKeyValueOrNull(const json& jsonObj, std::string_view key)
{
	if(auto it = jsonObj.find(key); it != jsonObj.end())
		return { it.value().template get<T>() };
	return { };
}

template<>
std::optional<json> GetJsonKeyValueOrNull<json>(const json& jsonObj, std::string_view key);

template<typename T>
T GetJsonKeyValue(const json& jsonObj, std::string_view key, std::optional<T> defaultValue = {})
{
	if(auto result = GetJsonKeyValueOrNull<T>(jsonObj, key); result.has_value())
		return result.value();
	if(!defaultValue.has_value())
		throw std::runtime_error(std::format("No such key: {} exists in the json provided, and default value is not given either", key));
	return std::move(defaultValue.value());
}
