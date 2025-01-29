# This script performs black box testing for build_master executable
# NOTE: We could have also implemented this test in Bash Scripts with Unix Tools such as diff but I found working with Python more promising.

import unittest
import subprocess
import sys
# For regular expression Matching
import re
import logging
import tempfile
import arttest
import os

class TestBase(arttest.ArtTest):
    def __init__(self, *args, **kwargs):
        super().__init__('build_master', *args, **kwargs)
        return

    def assert_string_matches_regex(self, string_data, regex_pattern):
        match = re.search(regex_pattern, string_data)
        self.assertIsNotNone(match, f'String \'{string_data}\' doesn\'t match the regex \'{regex_pattern}\'')
        return
    
    def tearDown(self):
        self.cleanup()
        return

class TestVersion(TestBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        return

    def test_version(self):
        output = self.run_with_args(["--version"])
        self.assertEqual(output.returncode, 0)  # Check program exited normally
        # build_mater --version
        # Build Master 1.0.0
        # Build Type: Debug
        self.assert_string_matches_regex(output.stdout[0], r'^Build Master \d+\.\d+\.\d+$')
        self.assert_string_matches_regex(output.stdout[1], r'^Build Type: (Debug|Release)$')
        self.assertIsNone(output.stderr)
        return

    # Geneates meson.build file and Runs meson into the initialized directory to check if the meson.build file is a valid build script
    def check_meson_build_script(self, directory = None):
        directory_arg = [f'--directory={directory}'] if directory else []
        output = self.run_with_args(['--update-meson-build'] + directory_arg)
        self.assertEqual(output.returncode, 0)
        self.assertIsNone(output.stderr)
        output.assert_exists_file(os.path.join(directory if directory else '', 'meson.build'))

        output = self.run_with_args(directory_arg + ['meson', 'setup', 'build'])
        self.assertEqual(output.returncode, 0)
        self.assertIsNone(output.stderr)
        return

    def run_test_init(self, is_cpp = False):
        output = self.run_with_args(['init', '--name=MyProject', '--canonical_name=myproject'] + (['--create-cpp'] if is_cpp else []))
        self.assertEqual(output.returncode, 0)
        self.assertIsNone(output.stderr)

        output.assert_exists_file('build_master.json')
        output.assert_exists_dir('source')
        output.assert_exists_dir('include')
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

    def run_test_init_directory(self, is_cpp = False):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = self.run_with_args(['init', '--name=MyProject', '--canonical_name=myproject', f'--directory={temp_dir}'] + ['--create-cpp'] if is_cpp else [])
            self.assertEqual(output.returncode, 0)
            self.assertIsNone(output.stderr)
            output.assert_exists_file(os.path.join(temp_dir, 'build_master.json'))
            output.assert_exists_dir(os.path.join(temp_dir, 'source'))
            output.assert_exists_dir(os.path.join(temp_dir, 'include'))
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