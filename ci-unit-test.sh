#!/bin/bash

build_master_meson --version
build_master --version
build_master --meson-build-template-path

echo "Running Primilary tests"
python ./unit_test/preliminary_tests.py -v
echo "Running Real World tests"
python ./unit_test/real_world_tests.py -v
