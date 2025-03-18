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

echo "Pulling origin ravi688-meson"
(cd $CLONE_PATH && git fetch && git checkout ravi688-meson && git pull origin ravi688-meson --ff)


# --------------- If build subdir doesn't exist creat one ---------------
# ----------------And if alredy exists then cleanup old artifacts -------
ZIPAPP_NAME=build_master_meson.pyz
ZIPAPP_OUTPUT_PATH="${BUILD_PATH}/${ZIPAPP_NAME}"

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

# ----------------- Create Zipapp '${ZIPAPP_NAME}' ---------------
INSTALL_PATH="${INSTALL_PREFIX}/bin/"
CREATE_ZIPAPP_PY="${CLONE_PATH}/packaging/create_zipapp.py"

echo "Creating zipapp"
$CREATE_ZIPAPP_PY --outfile $ZIPAPP_OUTPUT_PATH --interpreter '/usr/bin/env python3' $CLONE_PATH

# ------------------ Now copy that to final install path -------------
echo "Copying to $INSTALL_PATH"
cp $ZIPAPP_OUTPUT_PATH $INSTALL_PATH

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	chmod +x "${INSTALL_PATH}/${ZIPAPP_NAME}"
fi
