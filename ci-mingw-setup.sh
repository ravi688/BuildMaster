pacman -Sy --noconfirm mingw-w64-x86_64-meson \
	 mingw-w64-x86_64-python \
	 mingw-w64-x86_64-cmake \
	 mingw-w64-x86_64-spdlog \
	 mingw-w64-x86_64-python-pip

./install_meson.sh
./ci-build.sh

pacman -Sy mingw-w64-x86_64-zlib mingw-w64-x86_64-mbedtls
git clone https://github.com/ravi688/IXWebSocket.git
cd IXWebSocket
cmake -S . -B build -GNinja -DCMAKE_INSTALL_PREFIX=/mingw64 -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
