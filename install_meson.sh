#! /usr/bin/bash

# example: CLONE_DIR=<my clone path> INSTALL_PREFIX=/usr ./install_meson.sh


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
	git clone $GIT_REPO_PATH $CLONE_PATH
fi

# Check appropriate branch and update it with upstream
echo "Pulling origin ravi688-meson"
(cd $CLONE_PATH && git fetch && git checkout ravi688-meson && git pull origin ravi688-meson --ff)


# ---------------- Install build_master_meson ----------------------

# Platform detection
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
	PLATFORM="MINGW"
else
	PLATFORM="LINUX"
fi

# Make sure pyinstaller is available
if ! command -v pyinstaller &> /dev/null; then
	echo "pyinstaller not avaiable, installing it using pip"
	pip install --break-system-packages pyinstaller
fi

# Package meson into one executable
(cd $CLONE_PATH && pyinstaller --onefile --runtime-hook=runtime_hook.py --add-data "mesonbuild:mesonbuild" meson.py)

if [ -z $INSTALL_PREFIX ]; then
	INSTALL_PREFIX="/usr"
fi

# Copy the executable to the install directory
if [[ "$PLATFORM" == "MINGW" ]]; then
    echo "Copying dist/meson.exe to ${INSTALL_PREFIX}/bin/build_master_meson.exe"
    (cd $CLONE_PATH && cp dist/meson.exe "${INSTALL_PREFIX}/bin/build_master_meson.exe")
else
    echo "Copying dist/meson to ${INSTALL_PREFIX}/bin/build_master_meson"
    (cd $CLONE_PATH && cp dist/meson "${INSTALL_PREFIX}/bin/build_master_meson")
fi
