#!/usr/bin/python
# -*- coding: utf-8 -*-

import unittest
import os.path
import os

DIR = os.path.dirname(__file__) + os.sep 

file_list = [ele for ele in os.listdir(DIR) if os.path.isfile(DIR + os.sep + ele)]
test_list = [ele for ele in file_list if (ele.startswith('test') and \
                                          ele.endswith('.py'))]
modules_name = [filename.split('.')[0] for filename in test_list]


if __name__ == "__main__":
    suite = unittest.TestSuite()
    modules = [__import__(module_name, globals(), locals(),['*']) \
        for module_name in modules_name]

    #Load all test cases
    for module in modules:
        suite = unittest.findTestCases(module)
        unittest.TextTestRunner().run(suite)


