#!/bin/bash

apt-get update
apt-get install -y build-essential
apt-get install -y meson cmake
apt-get install python3
apt-get install python-is-python3
apt-get install libspdlog-dev

chmod +x ./install_meson.sh
./install_meson.sh
chmod +x ./ci-build.sh
./ci-build.sh
