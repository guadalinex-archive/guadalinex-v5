#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
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

class MSDDevicesTab:
    def __init__(self, prefs_obj, mcontroller):
        self.xml = prefs_obj.xml
        self.conf = prefs_obj.conf
        self.prefs_obj = prefs_obj

        #Devices Tab
        self.d_main_hbox = self.xml.get_widget("devices_main_vbox")
        self.devices_main_image = self.xml.get_widget("devices_main_image")
        self.devices_main_image.set_from_file(os.path.join(MSD.icons_files_dir,"devices_32x32.png"))
        #FIXED : UI
        self.device_conf = MobileManager.ui.MobileDeviceConfWidget()
        self.d_main_hbox.add(self.device_conf)
        
    def get_selected_device_name(self):
        dev = self.mcontroller.get_active_device()
        if dev == None :
            return _(u"Ningún dispositivo disponible")
        else:
            return dev.get_property("pretty-name")

