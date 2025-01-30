#!/bin/bash

if [ -n "${DEBUG}" ]; then
	meson setup build --buildtype=debug
else
	meson setup build --buildtype=release
fi
meson compile -C build
meson install -C build
