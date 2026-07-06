#!/bin/bash

PREFIX=${PREFIX:-/usr/local}
DSTPATH="$DESTDIR$PREFIX"
mkdir -p "${DSTPATH}/bin"

# Platform detection
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
        PLATFORM="MINGW"
else
        PLATFORM="LINUX"
        if [[ "$DSTPATH" == "/usr" || "$DSTPATH" == "/usr/*" ]] && [ "$EUID" -ne 0 ]; then
		echo $DSTPATH
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

MESON_CMD="$DSTPATH/bin/build_master_meson"

if INSTALL_PREFIX="$DSTPATH" ./yocto_install_meson.sh; then
	if [[ "$PLATFORM" == "MINGW" ]]; then
		($MESON_CMD setup build --reconfigure --buildtype=$BUILD_TYPE --prefix=$DSTPATH && \
		$MESON_CMD compile -C build && \
		$MESON_CMD install -C build --skip-subprojects)
	else
		($NO_ROOT $MESON_CMD setup build --reconfigure --buildtype=$BUILD_TYPE --prefix=$DSTPATH && \
		$NO_ROOT $MESON_CMD compile -C build && \
		$MESON_CMD install -C build --skip-subprojects)
	fi
fi
