#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica Móviles España S.A.U.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import gtk
import gtk.glade
import MSD
import os
import MobileManager.ui

class MSDGeneralTab:
    def __init__(self, prefs_obj, mcontroller):
        self.xml = prefs_obj.xml
        self.conf = prefs_obj.conf
        self.prefs_obj = prefs_obj

        #General Tab
        self.general_main_image = self.xml.get_widget("general_main_image")
        self.general_startup_checkbutton = self.xml.get_widget("general_startup_checkbutton")
        self.general_rss_checkbutton = self.xml.get_widget("general_rss_checkbutton")
        self.show_rss_feeder_button = self.xml.get_widget("show_rss_feeder_button")
        self.general_main_image.set_from_file(os.path.join(MSD.icons_files_dir,"general_32x32.png"))

        if os.path.exists(MSD.startup_file) :
            self.general_startup_checkbutton.set_active(True)
        else:
            self.general_startup_checkbutton.set_active(False)

        self.general_rss_checkbutton.set_active(self.conf.get_rss_on_connect())

    def connect_signals(self):
        self.show_rss_feeder_button.connect("clicked", self.__show_rss_feeder_button_cb, None)
        self.general_startup_checkbutton.connect("toggled", self.__general_startup_checkbutton_cb, None)
        self.general_rss_checkbutton.connect("toggled", self.__general_rss_checkbutton_cb, None)

    def __show_rss_feeder_button_cb(self, widget, data):
        #FIXME (7.0) : Better way close prefs
        self.prefs_obj.close_button.emit("clicked")
        self.prefs_obj.main_window_obj.rss.show_rss()

    def __general_startup_checkbutton_cb(self, widget, data):
        if widget.get_active() == True :
            os.system("touch %s" % MSD.startup_file)
        else:
            os.system("rm %s" % MSD.startup_file)
        
    def __general_rss_checkbutton_cb(self, widget, data):
        self.conf.set_rss_on_connect(widget.get_active())

