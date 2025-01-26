# This script performs black box testing for build_master executable
# NOTE: We could have also implemented this test in Bash Scripts with Unix Tools such as diff but I found working with Python more promising.

import unittest
import subprocess
import sys
# For regular expression Matching
import re
import logging

class ExecutableTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Path to your executable - modify as needed
        self.executable = "build_master"

    def run_with_args(self, args):
        """
        Run the executable with given input and return its output
        """
        process = subprocess.Popen(
            [self.executable] + args,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        stdout, stderr = process.communicate()
        return stdout.split('\n'), stderr.split('\n') if len(stderr) > 0 else None, process.returncode

    def assert_string_matches_regex(self, string_data, regex_pattern):
        match = re.search(regex_pattern, string_data)
        self.assertIsNotNone(match, f'String \'{string_data}\' doesn\'t match the regex \'{regex_pattern}\'')

    def test_version(self):
        stdout, stderr, returncode = self.run_with_args(["--version"])
        self.assertEqual(returncode, 0)  # Check program exited normally
        # build_mater --version
        # Build Master 1.0.0
        # Build Type: Debug
        self.assert_string_matches_regex(stdout[0], r'^Build Master \d+\.\d+\.\d+$')
        self.assert_string_matches_regex(stdout[1], r'^Build Type: (Debug|Release)$')
        self.assertIsNone(stderr)

if __name__ == '__main__':
    unittest.main()