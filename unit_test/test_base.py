
# For regular expression Matching
import re
import arttest
import tempfile
import os

class TestBase(arttest.ArtTest):
    def __init__(self, *args, **kwargs):
        super().__init__('build_master', *args, **kwargs)
        return

    def assert_string_matches_regex(self, string_data, regex_pattern):
        match = re.search(regex_pattern, string_data)
        self.assertIsNotNone(match, f'String \'{string_data}\' doesn\'t match the regex \'{regex_pattern}\'')
        return
    
    # Geneates meson.build file and Runs meson into the initialized directory to check if the meson.build file is a valid build script
    def check_meson_build_script(self, directory = None):
        directory_arg = [f'--directory={directory}'] if directory else []
        output = self.run_with_args(['--update-meson-build', '--force'] + directory_arg)
        self.assertEqual(output.returncode, 0)
        output.assert_exists_file(os.path.join(directory if directory else '', 'meson.build'))

        output = self.run_with_args(directory_arg + ['meson', 'setup', 'build'])
        if not output.returncode == 0:
            print(output.stdout)
            print(output.stderr)
        self.assertEqual(output.returncode, 0)
        return

    def tearDown(self):
        self.cleanup()
        return
