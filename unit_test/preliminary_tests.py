# This script performs black box testing for build_master executable
# NOTE: We could have also implemented this test in Bash Scripts with Unix Tools such as diff but I found working with Python more promising.

import unittest
import sys
import logging
import tempfile
import test_base
import os

class PreliminaryTests(test_base.TestBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        return

    def test_version(self):
        output = self.run_with_args(["--version"])
        self.assert_return_success(output)  # Check program exited normally
        # build_mater --version
        # Build Master 1.0.0
        # Build Type: Debug
        self.assert_string_matches_regex(output.stdout[0], r'^Build Master \d+\.\d+\.\d+$')
        self.assert_string_matches_regex(output.stdout[1], r'^Build Type: (Debug|Release)$')
        self.assertIsNone(output.stderr)
        return

    # Initialize a new project and check if the files exist which are supposed to exist
    def run_test_init(self, is_cpp = False):
        output = self.run_with_args(['init', '--name=MyProject', '--canonical_name=myproject'] + (['--create-cpp'] if is_cpp else []))
        self.assert_return_success(output)
        self.assertIsNone(output.stderr)

        output.assert_exists_file('build_master.json')
        output.assert_exists_dir('source')
        output.assert_exists_dir('include')
        output.assert_exists_dir('include/myproject')
        output.assert_exists_file('include/myproject/' + ('api_defines.hpp' if is_cpp else 'api_defines.h'))
        output.assert_exists_file('include/myproject/' + ('defines.hpp' if is_cpp else 'defines.h'))
        output.assert_exists_file('source/main.cpp' if is_cpp else 'source/main.c')

        self.check_meson_build_script()

        self.cleanupArtifacts()
        return

    # build_master init --name=MyProject --canonical_name=myproject
    def test_init(self):
        self.run_test_init()
        return

    # build_master init --name=MyProject --canonical_name=myproject --create-cpp
    def test_int_cpp(self):
        self.run_test_init(True)
        return

    # Initialize a new project into a directory and check if the files exist which are supposed to exist
    def run_test_init_directory(self, is_cpp = False):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = self.run_with_args(['init', '--name=MyProject', '--canonical_name=myproject', f'--directory={temp_dir}'] + ['--create-cpp'] if is_cpp else [])
            self.assert_return_success(output)
            self.assertIsNone(output.stderr)
            output.assert_exists_file(os.path.join(temp_dir, 'build_master.json'))
            output.assert_exists_dir(os.path.join(temp_dir, 'source'))
            output.assert_exists_dir(os.path.join(temp_dir, 'include'))
            output.assert_exists_dir(os.path.join(temp_dir, 'include', 'myproject'))
            output.assert_exists_file(os.path.join(temp_dir, 'include', 'myproject', 'api_defines.hpp' if is_cpp else 'api_defines.h'))
            output.assert_exists_file(os.path.join(temp_dir, 'include', 'myproject', 'defines.hpp' if is_cpp else 'defines.h'))
            output.assert_exists_file(os.path.join(temp_dir, 'source/main.cpp' if is_cpp else 'source/main.c'))
            self.check_meson_build_script(temp_dir)
            self.cleanupArtifacts()
        return

    # build_master init --name=MyProject --canonical_name=myproject --directory=<some temp dir>
    def test_init_directory(self):
        self.run_test_init_directory()
        return

    # build_master init --name=MyProject --canonical_namoe=myproject --directory=<some temp dir> --create-cpp
    def test_init_directory(self):
        self.run_test_init_directory(True)
        return

if __name__ == '__main__':
    unittest.main()