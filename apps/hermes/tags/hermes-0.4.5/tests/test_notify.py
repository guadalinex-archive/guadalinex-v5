#!/usr/bin/python
# -*- coding: utf8 -*-

# Configure path
import sys
sys.path.append('../')

import unittest
import time
import os.path

from utils.notification import NotificationDaemon
from gettext import gettext as _
ICON = os.path.abspath('test_icon.svg')
TIMEOUT = 0.3

class NotifyTest(unittest.TestCase):

    def setUp(self):
        self.msg_render = NotificationDaemon()

    def test_show_stock_icon(self):
        """Testing NotificationDaemon.show method with stock icons
        """

        actions = {"Click me": None}
        # Test stock icons
        nid = self.msg_render.show(_("Test"), _("Test message"), 
                "gtk-dialog-info", actions = actions)
        self.failUnless(isinstance(nid, int))
        time.sleep(TIMEOUT)
        self.msg_render.close(nid)


    def test_show_file_icon(self):
        """Testing NotificationDaemon.show method with icon file 
        """

        actions = {"Click me": None}
        # Test stock icons
        nid = self.msg_render.show(_("Test"), _("Test message"), 
                ICON, actions = actions)
        self.failUnless(isinstance(nid, int))
        time.sleep(TIMEOUT)
        self.msg_render.close(nid)


    def test_info(self):
        """Testing NotificationDaemon.show_info method
        """

        nid = self.msg_render.show_info(_("Information"), _("Test message"))
        self.failUnless(isinstance(nid, int))
        time.sleep(TIMEOUT)
        self.msg_render.close(nid)


    def test_warning(self):
        """Testing NotificationDaemon.show_info method
        """

        nid = self.msg_render.show_warning(_("Warning"), _("Test message"))
        self.failUnless(isinstance(nid, int))
        time.sleep(TIMEOUT)
        self.msg_render.close(nid)


    def test_error(self):
        """Testing NotificationDaemon.show_info method
        """
        nid = self.msg_render.show_error(_("Error"), _("Test message"))
        self.failUnless(isinstance(nid, int))
        time.sleep(TIMEOUT)
        self.msg_render.close(nid)

if __name__ == "__main__":
    unittest.main()
