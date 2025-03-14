#!/bin/bash

build_master_meson --version
build_master --version
build_master --meson-build-template-path

python ./unit_test/preliminary_tests.py -v
