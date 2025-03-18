
#include <iostream>
#include <invoke/invoke.hpp>
#include <cassert>

static constexpr std::string_view gMesonPYZName = "build_master_meson.pyz";
static constexpr std::string_view gPython = "python";

static std::string SelectPath(const std::vector<std::string>& paths)
{
	#ifdef _WIN32
		for(const auto& path : paths)
			if(path.find("mingw") != std::string::npos)
				return path;
		std::cerr << "Error: Couldn't find mingw python, probably you should run the following:\n"
			"pacman -S mingw-w64-x86_64-python\n";
		exit(EXIT_FAILURE);
	#else
		return paths[0];
	#endif
}

static void ReplaceSubstr(std::string& str, const std::string_view oldSubStr, const std::string_view newSubStr)
{
	std::size_t pos = 0;
	// Search for the word in the string
	while ((pos = str.find(oldSubStr, pos)) != std::string::npos)
	{
		// Replace the found word with the new word
		str.replace(pos, oldSubStr.length(), newSubStr);
		// Move past the word just replaced
		pos += newSubStr.length();
	}
}

#include <cstring>

int main(int argc, const char** argv)
{
	// Prepare arguments
  	std::vector<std::string> args;
  	args.reserve(argc + 1);

  	// Get the path of the python command (gPython)
  	auto fullPythonExePaths = invoke::GetExecutablePaths(gPython);
  	if(!fullPythonExePaths)
  	{
  		std::cerr << "Error: Failed to get absolute path of " << gPython << "\n";
  		exit(EXIT_FAILURE);
  	}

  	auto pythonPath = SelectPath(fullPythonExePaths.value());
  	args.push_back(pythonPath);

  	// Get the path of the command (gMesonPYZName)
  	auto fullMesonExePaths = invoke::GetExecutablePaths(gMesonPYZName);
  	if(!fullMesonExePaths)
  	{
  		std::cerr << "Error: Faild to get absolute path of "<< gMesonPYZName << "\n";
  		exit(EXIT_FAILURE);
  	}

  	// Now the overall command line arguments would be as follows:
  	// python build_master_meson.pyz argv[1] argv[2] ...
  	args.push_back(fullMesonExePaths.value()[0]);
  	for(int i = 1; i < argc; ++i)
  		args.push_back(std::string { argv[i] });

	 for(auto& arg : args)
	 {
	 	ReplaceSubstr(arg, "\\", "/");
	 	std::puts(arg.data());
	 }

	std::cout << "----------\n";

	for(auto& arg : args)
		std::cout << arg << "\n";

  	// Now run the python command, it will now run the meson pyzipapp
  	auto returnCode = invoke::Exec(args);
  	std::cout << "Return Code: " << returnCode << "\n";
  	exit(returnCode);
}

