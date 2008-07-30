#!/usr/bin/python
# -*- coding: UTF-8 -*-
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

import MSD

class MSDAIntranet(MSD.MSDAction):
    def __init__(self, act_manager, conf, conn_manager):
        print "Init MSDAIntranet"
        MSD.MSDAction.__init__(self, act_manager, conf, conn_manager)
        
        self.browser_by_default_checkbutton = self.get_prefs_widget("browser_by_default_checkbutton")
        self.initial_webpage_entry = self.get_prefs_widget("initial_webpage_entry")

        self.initial_webpage_entry.set_text(self.get_conf_key_value("url"))
        
        if self.get_conf_key_value("open_browser_by_default") == True :
            self.browser_by_default_checkbutton.set_active(True)
        else:
            self.browser_by_default_checkbutton.set_active(False)
            self.initial_webpage_entry.set_sensitive(False)

        self.browser_by_default_checkbutton.connect("toggled", self.__browser_checkbutton_cb, None)
        self.initial_webpage_entry.connect("changed", self.__initial_webpage_entry_cb, None)

    def __browser_checkbutton_cb (self, widget, data):
        if self.browser_by_default_checkbutton.get_active() == True :
            self.set_conf_key_value("open_browser_by_default", True)
            self.initial_webpage_entry.set_sensitive(True)
        else:
            self.set_conf_key_value("open_browser_by_default", False)
            self.initial_webpage_entry.set_sensitive(False)

    def __initial_webpage_entry_cb(self, widget, data):
        self.set_conf_key_value("url", self.initial_webpage_entry.get_text())

    def get_default_conf(self):
        return {'name' : _(u"movistar Intranet"),
                'id' : 20,
                'tooltip' : _(u"Conecta con la intranet de tu empresa"),
                'open_browser_by_default' : False,
                'url' : '',
                'help_url' : 'em_50.htm#emintranet',
                'connection' : 'movistar Internet' ,
                'connection_mandatory' : True
                 }

    def launch_action(self):
        if self.get_conf_key_value("open_browser_by_default") == True:
            import os
            os.system("gnome-open %s " % self.get_conf_key_value("url"))
