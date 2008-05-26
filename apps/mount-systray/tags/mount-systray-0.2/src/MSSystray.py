# MSSystray.py

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

import gtk
import gettext
import config
import gobject
import pynotify
import os
import time

from MSConfig import MSConfig
from MSDeviceManager import MSDeviceManager

class MSSystray:
    def __init__(self, device_manager, conf):

        self.device_manager = device_manager
        self.conf = conf
        self.xml = None
        self.notification = None

        self.stock_icons = {"usb": "gnome-dev-removable-usb",
                            "cdrom": "gnome-dev-cdrom",
                            "floppy": "gnome-dev-floppy",
                            "media-player": "multimedia-player"}

        # Menu
        self.menu = gtk.Menu()
        self.menu_volumes = []
        # Quit
        menu_item = gtk.ImageMenuItem(gtk.STOCK_QUIT)
        menu_item.connect("activate", self.__quit_cb, None)
        menu_item.show()
        self.menu.prepend(menu_item)
        # Preferences
        menu_item = gtk.ImageMenuItem(gtk.STOCK_PREFERENCES)
        menu_item.connect("activate", self.__show_preferences_cb, None)
        menu_item.show()
        self.menu.prepend(menu_item)
        # Separator
        menu_item = gtk.SeparatorMenuItem()
        menu_item.show()
        self.menu.prepend(menu_item)
        # No devices
        self.no_devices_menu_item = gtk.MenuItem(_("No devices mounted"))
        self.menu.prepend(self.no_devices_menu_item)

        # Init Icon Factory
        self.icon_theme = gtk.icon_theme_get_default ()        

        # first time
        for volume in self.device_manager.get_volumes():
            if volume["is_mounted"] == 0:
                continue
            
            menu_item = gtk.ImageMenuItem(volume["human_name"])

            if self.icon_theme.has_icon(self.stock_icons[volume["type"]]):
                icon = gtk.image_new_from_icon_name(self.stock_icons[volume["type"]], gtk.ICON_SIZE_MENU)
                menu_item.set_image(icon)

            # Signal
            menu_item.connect("activate", self.__device_activate_cb, volume["uid"])

            menu_item.show()
            self.menu.prepend (menu_item)

            self.menu_volumes.append({"uid" : volume["uid"],
                                      "menu_item" : menu_item,
                                      })

        if len(self.menu_volumes) > 0:
            self.no_devices_menu_item.hide()
        else:
            self.no_devices_menu_item.show()

        self.sicon = gtk.StatusIcon()
        self.sicon.set_visible(True)
        if len(self.menu_volumes) < 1:
            self.sicon.set_from_icon_name("drive-removable-media")
            if self.conf.get_hide_systray():
                self.sicon.set_visible(False)
        else:
            self.sicon.set_from_icon_name("harddrive")
            if self.conf.get_blink_all_time():
                self.sicon.set_blinking(True)
            
        self.sicon.set_tooltip(_("Removable devices control"))

        # Signals
        self.sicon.connect("popup-menu", self.__sicon_popupmenu_cb, None)
        self.sicon.connect("activate", self.__sicon_activate_cb, None)
        self.device_manager.connect("volume_mounted", self.__volume_mounted_cb)
        self.device_manager.connect("volume_unmounted", self.__volume_unmounted_cb)
        self.device_manager.connect("volume_removed", self.__volume_unmounted_cb)

        # The timeout
        miliseconds = self.conf.get_minutes_warning() * 60 * 1000
        self.timeout_id = gobject.timeout_add(miliseconds, self.__timeout_cb)

    def __volume_mounted_cb(self, device_manager, uid, human_name, volume_type, data=None):
        menu_item = gtk.ImageMenuItem(human_name)
        if self.icon_theme.has_icon(self.stock_icons[volume_type]):
            icon = gtk.image_new_from_icon_name(self.stock_icons[volume_type], gtk.ICON_SIZE_MENU)
            menu_item.set_image(icon)

        menu_item.show()
        self.menu.prepend (menu_item)
        self.no_devices_menu_item.hide()
        self.sicon.set_from_icon_name("harddrive")
        self.sicon.set_visible(True)
        
        # Signal
        menu_item.connect("activate", self.__device_activate_cb, uid)

        self.menu_volumes.append({"uid" : uid,
                                  "menu_item" : menu_item,
                                  })

        if self.conf.get_show_notify_detected():
            if self.__is_hermes_running() == False:
                if volume_type != "cdrom":
                    msg = _("Remember umount it when you finish your work if you wan't to lose your information")
                else:
                    msg = ""

                self.show_notification(_("New device detected\n<small>(%s)</small>") % human_name,
                                       msg,
                                       self.stock_icons[volume_type])
            else:
                self.sicon.set_blinking(True)
                gobject.timeout_add(5000, self.__timeout_remove_blink_cb)

        if self.conf.get_blink_all_time():
            self.sicon.set_blinking(True)

        # Restart timeout
        gobject.source_remove(self.timeout_id)
        miliseconds = self.conf.get_minutes_warning() * 60 * 1000
        self.timeout_id = gobject.timeout_add(miliseconds, self.__timeout_cb)

    def __volume_unmounted_cb(self, device_manager, uid, data=None):
        for menu in self.menu_volumes:
            if menu["uid"] == uid:
                self.menu.remove(menu["menu_item"])
                self.menu_volumes.remove(menu)

        if len(self.menu_volumes) > 0:
            self.no_devices_menu_item.hide()
        else:
            self.sicon.set_from_icon_name("drive-removable-media")
            self.no_devices_menu_item.show()
            if self.conf.get_hide_systray():
                self.sicon.set_visible(False)
            if self.conf.get_blink_all_time():
                self.sicon.set_blinking(False)

    def __is_hermes_running(self):
        (i, o) = os.popen2("ps -eo user,cmd | grep hermes | grep %s" % os.environ["USER"])
        process_list = o.readlines()
        for x in process_list :
            tmp = x.split()
            if "hermes_hardware.py" in tmp or "./hermes_hardware.py" in tmp:
                return True
        return False
            
    def __sicon_popupmenu_cb(self, status_icon, button, activate_time, data):
        self.menu.popup(None, None, gtk.status_icon_position_menu, button, activate_time, status_icon)

    def __sicon_activate_cb(self, status_icon, data):
        self.menu.popup(None, None, gtk.status_icon_position_menu, 1, gtk.get_current_event_time(), status_icon)

    def __device_activate_cb(self, menu_item, uid):
        self.device_manager.volume_unmount(uid)

    def __show_preferences_cb(self, menu_item, user_data):
        if self.xml == None:
            self.xml = gtk.glade.XML(config.getGladedir() + "/preferences.glade")
            self.preferences = self.xml.get_widget ("preferences_window")
            widget = self.xml.get_widget("close_button")
            widget.connect ("clicked", self.__close_button_clicked_cb, None)
            self.__populate_preferences()

        self.preferences.show_all()

    def __close_button_clicked_cb(self, widget, user_data):
        self.preferences.hide()

    def __quit_cb(self, menu_item, user_data):
        gobject.source_remove(self.timeout_id)
        if (self.notification):
            self.notification.close()
        gtk.main_quit()

    def __populate_preferences(self):
        minutes_spinbutton = self.xml.get_widget("minutes_spinbutton")
        show_notify_checkbutton = self.xml.get_widget("show_notify_checkbutton")
        new_device_checkbutton = self.xml.get_widget("new_device_checkbutton")
        hide_systray_checkbutton = self.xml.get_widget("hide_systray_checkbutton")
        blink_checkbutton = self.xml.get_widget("blink_checkbutton")

        # Saved conf
        minutes_spinbutton.set_value(self.conf.get_minutes_warning())
        show_notify_checkbutton.set_active(self.conf.get_show_notify())
        new_device_checkbutton.set_active(self.conf.get_show_notify_detected())
        hide_systray_checkbutton.set_active(self.conf.get_hide_systray())
        blink_checkbutton.set_active(self.conf.get_blink_all_time())

        # Signals
        minutes_spinbutton.connect("value_changed", self.__minutes_changed_cb, None)
        show_notify_checkbutton.connect("toggled", self.__show_notify_cb, None)
        new_device_checkbutton.connect("toggled", self.__new_device_cb, None)
        hide_systray_checkbutton.connect("toggled", self.__hide_systray_cb, None)
        blink_checkbutton.connect("toggled", self.__blink_all_time_cb, None)

    def __blink_all_time_cb(self, checkbutton, data_user):
        self.conf.set_blink_all_time (checkbutton.get_active())
        if (checkbutton.get_active()):
            if len(self.menu_volumes) > 0:
                self.sicon.set_blinking(True)
            else:
                self.sicon.set_blinking(False)
        else:
            self.sicon.set_blinking(False)

    def __minutes_changed_cb(self, minutes_spinbutton, data_user):
        minutes_spinbutton.update()
        self.conf.set_minutes_warning (int(minutes_spinbutton.get_value()))
        gobject.source_remove(self.timeout_id)
        miliseconds = self.conf.get_minutes_warning() * 60 * 1000
        self.timeout_id = gobject.timeout_add(miliseconds, self.__timeout_cb)

    def __show_notify_cb(self, checkbutton, data_user):
        self.conf.set_show_notify (checkbutton.get_active())

    def __new_device_cb(self, checkbutton, data_user):
        self.conf.set_show_notify_detected (checkbutton.get_active())

    def __hide_systray_cb(self, checkbutton, data_user):
        self.conf.set_hide_systray (checkbutton.get_active())
        if checkbutton.get_active():
            if len(self.menu_volumes) < 1:
                self.sicon.set_visible(False)
        else:
            self.sicon.set_visible(True)

    def __timeout_cb(self):
        if len(self.menu_volumes) > 0:
            self.sicon.set_blinking(True)
            if self.conf.get_show_notify():
                self.show_notification(_("Remember that ..."),
                                       _("You must unmount every removable device before extract it, if you wan't to lose your information"))
        gobject.timeout_add(5000, self.__timeout_remove_blink_cb)
        return True

    def __timeout_remove_blink_cb(self):
        if self.conf.get_blink_all_time() == False:
            self.sicon.set_blinking(False)

        return False

    def show_notification(self, title, msg, icon=None):
        self.notification = pynotify.Notification(title, msg, icon)
        self.notification.set_urgency(pynotify.URGENCY_LOW)
        # Expires in six seconds
        self.notification.set_timeout(6000)
        self.notification.set_property("status-icon", self.sicon)
	self.notification.show()
