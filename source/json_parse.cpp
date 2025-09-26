#include <build_master/json_parse.hpp>
#include <build_master/misc.hpp> // for LoadTextFile(), and GetBuildMasterJsonFilePath()

static std::optional<std::pair<std::size_t, std::size_t>> IsBlankLine(const std::string& str, std::size_t charIndex)
{
	std::size_t lineBegin;
	while(charIndex > 0)
	{
		char ch = str[--charIndex];
		if(ch == '\n')
		{
			lineBegin = charIndex;
			break;
		}
		if(!std::isspace(ch))
			return { };
	}
	std::size_t lineEnd;
	while(charIndex < str.size())
	{
		char ch = str[charIndex++];
		if(ch == '\n')
		{
			lineEnd = charIndex;
			break;
		}
		if(!std::isspace(ch))
			return { };
	}
	return { { lineBegin, lineEnd } };
}

static void EraseCppComments(std::string& str)
{
	std::size_t index;
	while((index = str.find("//")) != std::string::npos)
	{
		auto newLineIndex = str.find_first_of('\n', index + 2);
		str.erase(std::next(str.begin(), index), std::next(str.begin(), newLineIndex));
		if(auto result = IsBlankLine(str, index); result.has_value())
			str.erase(std::next(str.begin(), result->first), std::next(str.begin(), result->second));
	}
}

// directory: value passed to --directory flag
json ParseBuildMasterJson(std::string_view directory)
{
	std::string jsonStr = LoadTextFile(GetBuildMasterJsonFilePath(directory));
	EraseCppComments(jsonStr);
	json data = json::parse(jsonStr);
	return data;
}


template<>
std::optional<json> GetJsonKeyValueOrNull<json>(const json& jsonObj, std::string_view key)
{
	if(auto it = jsonObj.find(key); it != jsonObj.end())
		return { it.value() };
	return { };
}
