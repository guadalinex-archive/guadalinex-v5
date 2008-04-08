#!/usr/bin/python
# -*- coding: utf8 -*-

from devicelist import DeviceList

import unittest
from sets import Set

class DeviceListTest (unittest.TestCase):

    def setUp(self):
        pass

    def test_save_and_load(self):
        dl = DeviceList()
        dl.save('/tmp/devicelist_tmp_file')

        dl2 = DeviceList()
        dl2.load('/tmp/devicelist_tmp_file')

        dif = dl.symmetric_difference(dl2)
        self.assertEquals(dif, Set())




if __name__ == "__main__":
    unittest.main()


