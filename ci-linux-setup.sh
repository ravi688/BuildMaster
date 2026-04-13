#!/bin/bash

apt-get update
apt-get install -y build-essential
apt-get install -y meson cmake
apt-get install python3
apt-get install python-is-python3
apt-get install libspdlog-dev
apt-get install pip
apt-get install cmake

chmod +x ./install_meson.sh
./install_meson.sh
chmod +x ./ci-build.sh
./ci-build.sh


apt install libssl-dev
apt install libmbedtls-dev
git clone https://github.com/ravi688/IXWebSocket.git
cd IXWebSocket
cmake -S . -B build -GNinja -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
