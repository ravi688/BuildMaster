#include <iostream>

int main()
{
	std::cout << "Build Master 1.0.0\n";
	#ifdef BUILD_MASTER_RELEASE
		std::cout << "Build Type: Release\n";
	#else
		std::cout << "Build Type: Debug\n";
	#endif
	return 0;
}