#!/usr/bin/python
# -*- coding: utf-8 -*-

import unittest
import os

class TestMainProcess(unittest.TestCase):

    def test_all(self):
        command = "cd test_pkg;gcs_build > /dev/null;"
        command += "diff -ur --exclude=changelog* --exclude=.svn debian.orig/ debian/" 
        
        (stdint, stdout, stderr) = os.popen3(command)
        self.assertEqual(stderr.read(), '')
        self.assertEqual(stdout.read(), '')


if __name__ == "__main__":
    unittest.main()


