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


# Install meson (as build_master_meson)
pip install $CLONE_PATH
