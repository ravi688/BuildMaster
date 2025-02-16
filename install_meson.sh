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


# --------------- If build subdir doesn't exist creat one ---------------
# ----------------And if alredy exists then cleanup old artifacts -------
ZIPAPP_OUTPUT_PATH="${BUILD_PATH}/build_master_meson"

if [ -d $BUILD_PATH ]; then
	if [ -f $ZIPAPP_OUTPUT_PATH ]; then
		echo "${ZIPAPP_OUTPUT_PATH} already exists, removing it"
		rm $ZIPAPP_OUTPUT_PATH
	fi
else
	mkdir -p $BUILD_PATH
fi

if [ -z $INSTALL_PREFIX ]; then
	INSTALL_PREFIX="/usr"
fi

# ----------------- Create Zipapp 'build_master_meson' ---------------
INSTALL_PATH="${INSTALL_PREFIX}/bin/"
CREATE_ZIPAPP_PY="${CLONE_PATH}/packaging/create_zipapp.py"

echo "Creating zipapp"
$CREATE_ZIPAPP_PY --outfile $ZIPAPP_OUTPUT_PATH --interpreter '/usr/bin/env python3' $CLONE_PATH

# ------------------ Now copy that to final install path -------------
echo "Copying to $INSTALL_PATH"
cp $ZIPAPP_OUTPUT_PATH $INSTALL_PATH
