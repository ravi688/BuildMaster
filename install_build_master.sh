#!/bin/bash

# Platform detection
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
        PLATFORM="MINGW"
else
        PLATFORM="LINUX"
fi

if [ -n "${DEBUG}" ]; then
	BUILD_TYPE="debug"
else
	BUILD_TYPE="release"
fi

if ./install_meson.sh; then
	if [[ "$PLATFORM" == "MINGW" ]]; then
		(build_master_meson setup build --reconfigure --buildtype=$BUILD_TYPE && \
		build_master_meson compile -C build && \
		build_master_meson install -C build --skip-subprojects)
	else
		(sudo -u $SUDO_USER build_master_meson setup build --reconfigure --buildtype=$BUILD_TYPE && \
		sudo -u $SUDO_USER build_master_meson compile -C build && \
		build_master_meson install -C build --skip-subprojects)
	fi
fi
