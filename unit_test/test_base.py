
# For regular expression Matching
import re
import arttest
import tempfile

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
