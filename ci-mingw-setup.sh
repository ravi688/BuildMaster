pacman -Sy --noconfirm mingw-w64-x86_64-meson \
	 mingw-w64-x86_64-python \
	 mingw-w64-x86_64-cmake \
	 mingw-w64-x86_64-spdlog \
	 mingw-w64-x86_64-python-pip

./install_meson.sh
./ci-build.sh
