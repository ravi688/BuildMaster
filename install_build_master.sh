#!/bin/bash

if ./install_meson.sh; then
	(sudo -u $SUDO_USER build_master_meson setup build --reconfigure && \
	sudo -u $SUDO_USER build_master_meson compile -C build && \
	build_master_meson install -C build --skip-subprojects)
fi
