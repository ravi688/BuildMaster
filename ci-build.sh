#!/bin/bash

if [ -n "${DEBUG}" ]; then
	build_master_meson setup build --buildtype=debug
else
	build_master_meson setup build --buildtype=release
fi
build_master_meson compile -C build
build_master_meson install -C build
