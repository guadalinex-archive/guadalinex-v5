#!/usr/bin/python
# -*- coding: utf-8 -*-

import unittest
import sys

import gtk

sys.path.insert(0, '/usr/share/gsd')
from appinstall import DiskSelectorDialog

DISKS = [
        {'info.category': 'volume',
            'volume.label': 'GSD-Juegos'},
        {'info.category': 'volume',
            'volume.label': 'GSD-Desarrollo'}
        ]

if __name__ == '__main__':
    dialog = DiskSelectorDialog(DISKS)
    dialog.run()
    dialog.destroy()
    print dialog.get_selected()

