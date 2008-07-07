# MSConfig.py

# Copyright (c) 2007, Junta de Andalucia

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import os
import sys
import gconf
import gobject

class MSConfig(gobject.GObject):

    __gsignals__ = {
        'config_changed': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,))
        }
    
    def __init__(self):
        gobject.GObject.__init__(self)
        try:
            self.gconf_client = gconf.client_get_default()
            self.gconf_client.add_dir ("/apps/mount-systray", gconf.CLIENT_PRELOAD_NONE)
            self.gconf_client.notify_add ("/apps/mount-systray/click_on_device_action", self.__on_config_changed_cb, None)
            self.gconf_client.notify_add ("/apps/mount-systray/minutes_warning", self.__on_config_changed_cb, None)
            self.gconf_client.notify_add ("/apps/mount-systray/show_notify", self.__on_config_changed_cb, None)
            self.gconf_client.notify_add ("/apps/mount-systray/show_notify_detected", self.__on_config_changed_cb, None)
            self.gconf_client.notify_add ("/apps/mount-systray/hide_systray", self.__on_config_changed_cb, None)
            self.gconf_client.notify_add ("/apps/mount-systray/blink_all_time", self.__on_config_changed_cb, None)
        except:
            print _("Can't load gconf configuration, try to reinstall or logout.")
            sys.exit(1)

    def __on_config_changed_cb(self, client, cnxn_id, entry, user_data):
        self.emit("config_changed", entry)

    def get_click_on_device_action(self):
        return self.gconf_client.get_string("/apps/mount-systray/click_on_device_action")

    def set_click_on_device_action(self, value):
        self.gconf_client.set_string("/apps/mount-systray/click_on_device_action", value)

    def get_minutes_warning(self):
        return self.gconf_client.get_int("/apps/mount-systray/minutes_warning")

    def set_minutes_warning(self, value):
        self.gconf_client.set_int("/apps/mount-systray/minutes_warning", value)

    def get_show_notify(self):
        return self.gconf_client.get_bool("/apps/mount-systray/show_notify")

    def set_show_notify(self, value):
        self.gconf_client.set_bool("/apps/mount-systray/show_notify", value)

    def get_show_notify_detected(self):
        return self.gconf_client.get_bool("/apps/mount-systray/show_notify_detected")

    def set_show_notify_detected(self, value):
        self.gconf_client.set_bool("/apps/mount-systray/show_notify_detected", value)

    def get_hide_systray(self):
        return self.gconf_client.get_bool("/apps/mount-systray/hide_systray")

    def set_hide_systray(self, value):
        self.gconf_client.set_bool("/apps/mount-systray/hide_systray", value)

    def get_blink_all_time(self):
        return self.gconf_client.get_bool("/apps/mount-systray/blink_all_time")
        
    def set_blink_all_time(self, value):
        self.gconf_client.set_bool("/apps/mount-systray/blink_all_time", value)

gobject.type_register(MSConfig)
