# This script performs black box testing for build_master executable
# NOTE: We could have also implemented this test in Bash Scripts with Unix Tools such as diff but I found working with Python more promising.

import unittest
import subprocess
import sys
import logging
import tempfile
import test_base
import os
import logging

logging.basicConfig(level=logging.DEBUG)

git_repos = [
    'https://github.com/ravi688/HPML.git',
    'https://github.com/ravi688/MeshLib.git',
    'https://github.com/ravi688/PhyMacParser.git',
    'https://github.com/ravi688/SafeMemory.git',
    'https://github.com/ravi688/Common.git',
    'https://github.com/ravi688/GLSLCommon.git',
    'https://github.com/ravi688/DiskManager.git',
    'https://github.com/ravi688/BufferLib.git',
    'https://github.com/ravi688/Calltrace.git',
    'https://github.com/ravi688/MeshLib.git',
    'https://github.com/ravi688/ttf2mesh.git',
    'https://github.com/ravi688/NetSocket.git'
]

# git clone <url> into <directory>
# build_master --update-meson-build
# build_master meson setup build
class RealWorldTests(test_base.TestBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        return

    def clone_build_compile(self, https_url):
        logging.debug(f'URL: {https_url}')
        # create a temporary directory
        with tempfile.TemporaryDirectory() as temp_dir:
            # clone the respository
            output = self.run_cmd_with_args('git', ['clone', https_url, temp_dir])
            self.assertEqual(output.returncode, 0)
            # regenerate meson.build file from build_master.json
            # and configure the meson project to check if meson.build is a valid file            
            self.check_meson_build_script(temp_dir)
        logging.debug('PASS')
        return


    def test_all(self):
        logging.debug('\n')
        for https_url in git_repos:
            self.clone_build_compile(https_url)
        return

if __name__ == '__main__':
    unittest.main()