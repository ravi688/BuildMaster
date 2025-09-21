#! /usr/bin/bash

# example: CLONE_DIR=<my clone path> INSTALL_PREFIX=/usr ./install_meson.sh

# Stop executing the script if any command fails
set -e

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
	if is_in_docker || [ -z "${SUDO_USER-}]"; then
		NO_ROOT=""
	else
		NO_ROOT="sudo -u $SUDO_USER"
	fi
fi

# --------------- Git repository cloning --------------------------
REPO_NAME=meson
GIT_REPO_PATH="https://github.com/ravi688/${REPO_NAME}.git"
if [ -z $CLONE_DIR ]; then
	CLONE_DIR="./.${REPO_NAME}.clone_dst.dir"
fi

CLONE_PATH="${CLONE_DIR}/${REPO_NAME}"
BUILD_PATH="${CLONE_DIR}/build"

if [ -d $CLONE_PATH ]; then
	echo "The repo already seem to be cloned, skipping git clone"
else
	if [[ "$PLATFORM" == "MINGW" ]]; then
		git clone $GIT_REPO_PATH $CLONE_PATH
	else
		$NO_ROOT git clone $GIT_REPO_PATH $CLONE_PATH
	fi
fi

# Check appropriate branch and update it with upstream
echo "Pulling origin ravi688-meson"

if [[ "$PLATFORM" == "MINGW" ]]; then
	(cd $CLONE_PATH && git fetch \
			&& git checkout ravi688-meson \
			&& git pull origin ravi688-meson --ff)
else
	(cd $CLONE_PATH && $NO_ROOT git fetch \
		&& $NO_ROOT git checkout ravi688-meson \
		&& $NO_ROOT git pull origin ravi688-meson --ff)
fi


# ---------------- Install build_master_meson ----------------------
(cd $CLONE_PATH && chmod +x ./install.sh)
(cd $CLONE_PATH && ./install.sh)
