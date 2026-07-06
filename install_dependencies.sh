#!/bin/bash

# Platform detection
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
        PLATFORM="MINGW"
else
        PLATFORM="LINUX"
fi

if [[ "$PLATFORM" == "MINGW" ]]; then
	pacman -Sy --noconfirm mingw-w64-x86_64-meson \
	 	mingw-w64-x86_64-python \
	 	mingw-w64-x86_64-cmake \
	 	mingw-w64-x86_64-spdlog \
	 	mingw-w64-x86_64-python-pip
else
	apt-get update
	apt-get install -y build-essential
	apt-get install -y meson cmake
	apt-get install -y python3
	apt-get install -y python-is-python3
	apt-get install -y libspdlog-dev
	apt-get install -y python3-pip
	apt-get install -y pkgconf

fi
