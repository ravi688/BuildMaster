#!/bin/bash


build_master_meson --version
build_master --version

python ./unit_test/unit_test.py
