# This script performs black box testing for build_master executable
# NOTE: We could have also implemented this test in Bash Scripts with Unix Tools such as diff but I found working with Python more promising.

import unittest
import subprocess
import sys
# For regular expression Matching
import re
import logging
import tempfile
import os
import shutil

def delete_all_files_and_dirs(directory):
    # Loop through all items in the directory
    for filename in os.listdir(directory):
        file_path = os.path.join(directory, filename)
        
        if os.path.isdir(file_path):  # If it's a directory, remove it recursively
            shutil.rmtree(file_path)
        else:  # If it's a file, remove it
            os.remove(file_path)

class ArtOutput(unittest.TestCase):
    def __init__(self, working_dir, stdout, stderr, returncode):
        self._working_dir = working_dir
        self._stdout = stdout
        self._stderr = stderr
        self._returncode = returncode
        return
    # List of lines printed on stdout
    @property
    def stdout(self):
        return self._stdout
    # Either None or list of lines printed on stderr
    @property
    def stderr(self):
        return self._stderr
    # Return code
    @property
    def returncode(self):
        return self._returncode
    # Returns True if the file existss at rel_file_path relative to the _working_dir, otherwise False
    def exists_file(self, rel_file_path):
        file_path = os.path.join(self._working_dir, rel_file_path)
        return os.path.exists(file_path)
    # Returns True if the dir existss at rel_dir_path relative to the _working_dir, otherwise False
    def exists_dir(self, rel_dir_path):
        dir_path = os.path.join(self._working_dir, rel_dir_path)
        return os.path.exists(dir_path)
    # Assertion Passes if the file exists at rel_file_path relative to the _working_dir, otherwise False
    def assert_exists_file(self, rel_file_path):
        self.assertTrue(self.exists_file(rel_file_path), f'File \'{rel_file_path}\' is expected to exist but it doesn\'t')
        return
    # Assertion Passes if the dir exists at rel_dir_path relative to the _working_dir, otherwise False
    def assert_exists_dir(self, rel_dir_path):
        self.assertTrue(self.exists_dir(rel_dir_path), f'Directory \'{rel_dir_path}\' is expected to exist but it doesn\'t')
        return

class ArtTest(unittest.TestCase):
    def __init__(self, executable, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Path to your executable - modify as needed
        self._executable = executable
        self._working_dir = tempfile.TemporaryDirectory()
        return

    def cleanup(self):
        self._working_dir.cleanup()
        return
    # Be careful when calling this function, the _working_dir must be temporary and only contain artifacts generated by the executable being tested
    # , all files or sub directories within it will be deleted recursively.
    def cleanupArtifacts(self):
        delete_all_files_and_dirs(self._working_dir.name)
        return

    def run_cmd_with_args(self, executable, args):
        """
        Run the executable with given input and return its output
        """
        process = subprocess.Popen(
            [executable] + args,
            cwd=self._working_dir.name,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        stdout, stderr = process.communicate()
        return ArtOutput (self._working_dir.name, stdout.split('\n'), stderr.split('\n') if len(stderr) > 0 else None, process.returncode)
    
    def assert_return_success(self, output: ArtOutput):
        self.assertEqual(output.returncode, 0, f'stdout: {output.stdout}\nstderr: {output.stderr}')
        return
 
    def run_with_args(self, args):
        return self.run_cmd_with_args(self._executable, args)

    def assert_string_matches_regex(self, string_data, regex_pattern):
        match = re.search(regex_pattern, string_data)
        self.assertIsNotNone(match, f'String \'{string_data}\' doesn\'t match the regex \'{regex_pattern}\'')

    def assert_string_matches_any_regex(self, string_list : [str], regex_pattern : str):
        isMatchFound : bool = False
        for _str in string_list:
            match = re.search(regex_pattern, _str)
            if match:
                isMatchFound = True
                break
        self.assertTrue(isMatchFound, f'No match found for regex: \'{regex_pattern}\'');
        return