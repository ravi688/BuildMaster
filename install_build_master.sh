#!/bin/bash

# Platform detection
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
        PLATFORM="MINGW"
else
        PLATFORM="LINUX"
        if [ "$EUID" -ne 0 ]; then
                echo "This script must be run as root. Please use sudo."
                exit -1
        fi

        is_in_docker() {
                [ -f "/.dockerenv" ] || grep -qa docker /proc/1/cgroup
        }

        # Detect if we are running on docker container
        if is_in_docker || [ -z "${SUDO_USER-}" ]; then
                NO_ROOT=""
        else
                NO_ROOT="sudo -u $SUDO_USER"
        fi
fi

if [ -n "${DEBUG}" ]; then
	BUILD_TYPE="debug"
else
	BUILD_TYPE="release"
fi

./install_dependencies.sh

if ./install_meson.sh; then
	if [[ "$PLATFORM" == "MINGW" ]]; then
		(build_master_meson setup build --reconfigure --buildtype=$BUILD_TYPE && \
		build_master_meson compile -C build && \
		build_master_meson install -C build --skip-subprojects)
	else
		($NO_ROOT build_master_meson setup build --reconfigure --buildtype=$BUILD_TYPE && \
		$NO_ROOT build_master_meson compile -C build && \
		build_master_meson install -C build --skip-subprojects)
	fi
fi
