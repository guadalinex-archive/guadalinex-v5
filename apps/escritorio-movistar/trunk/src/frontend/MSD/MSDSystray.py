#!/usr/bin/python
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
import dbus
import dbus.glib
import MSD
from MSDUtils import *
from PPPManager import *

class MSDSystray:
    def __init__(self, conf, main_window, conn_manager, act_manager, mcontroller):

        self.conf = conf
        self.main_window = main_window
        self.mcontroller = mcontroller
        self.conn_manager = conn_manager
        self.act_manager = act_manager
        
        self.sicon = gtk.StatusIcon()
        self.sicon.set_from_file(MSD.icons_files_dir + "no-conectado.png")
        self.sicon.set_visible(True)
        self.sicon.set_tooltip(u"Escritorio movistar: no conectado")

        # Menu
        self.xml = gtk.glade.XML(MSD.glade_files_dir + "systray_menu.glade")
        self.menu = self.xml.get_widget("systray_menu")
        self.exit = self.xml.get_widget("systray_exit")
        self.connect = self.xml.get_widget ("systray_connect")
        self.disconnect = self.xml.get_widget ("systray_disconnect")
        self.open = self.xml.get_widget ("systray_open")

        # Actions
        self.acts_items = []
        self.reload_actions()

        # Hide items
        self.disconnect.hide()
        self.update_installed_actions_items()

        # DBus
        self.__connect_to_bus()

        # Signals
        self.sicon.connect("activate", self.__sicon_activate_cb, None)
        self.sicon.connect("popup-menu", self.__sicon_popupmenu_cb, None)

        dict = {"on_systray_exit_activate" : self.__on_systray_exit_activate,
                "on_systray_connect_activate" : self.__on_systray_connect_activate,
                "on_systray_disconnect_activate": self.__on_systray_disconnect_activate,
                "on_systray_open_activate" : self.__on_systray_open_activate
                }
        self.xml.signal_autoconnect(dict)

    def __connect_to_bus(self):
        if self.mcontroller.dialer != None :
            # signals
            self.ppp_manager = self.mcontroller.dialer
            self.ppp_manager.connect("connected", self.__connected_cb)
            self.ppp_manager.connect("disconnected", self.__disconnected_cb)

    def __connected_cb(self, dialer):
        self.connect.hide()
        self.disconnect.show()
        self.sicon.set_from_file(MSD.icons_files_dir + "conectado.png")
        self.sicon.set_tooltip(u"Escritorio movistar:conectado")
        
    def __disconnected_cb(self, dialer):
        self.connect.show()
        self.disconnect.hide()
        self.sicon.set_from_file(MSD.icons_files_dir + "no-conectado.png")
        self.sicon.set_tooltip(u"Escritorio movistar: no conectado")

    def __action_activate_cb(self, menu_item, codename):
        self.act_manager.launch_action(codename)

    def __sicon_activate_cb(self, status_icon, data):
        if self.conf.get_ui_general_key_value ("systray_showing_mw"):
            self.main_window.hide()
            self.conf.set_ui_general_key_value("systray_showing_mw", False)
            self.conf.save_conf()
        else:
            self.main_window.show()
            self.conf.set_ui_general_key_value("systray_showing_mw", True)
            self.conf.save_conf()

    def __sicon_popupmenu_cb(self, status_icon, button, activate_time, data):
        self.menu.popup(None, None, gtk.status_icon_position_menu, button, activate_time, status_icon)

    def __on_systray_exit_activate(self, widget):
        close_app(self.conn_manager, self.conf)

    def __on_systray_connect_activate(self, widget):
        ret = self.conn_manager.connect_to_connection()
        if ret == False:
            self.conn_manager.error_on_connection()

    def __on_systray_disconnect_activate(self, widget):
        self.conn_manager.disconnect()

    def __on_systray_open_activate(self, widget):
        self.main_window.deiconify()
        self.main_window.show()
        self.conf.set_ui_general_key_value("systray_showing_mw", True)
        self.conf.save_conf()

    def update_installed_actions_items(self):
        for action_item in self.acts_items:
            menu_item = action_item["menu_item"]
            installed_value = self.conf.get_action_key_value(action_item["codename"], "installed")
            if installed_value == False:
                if self.conf.get_hide_uninstalled_services():
                    menu_item.hide()
                else:
                    menu_item.show()
                    menu_item.set_sensitive(False)
            else:
                menu_item.show()
                menu_item.set_sensitive(True)

    def reload_actions(self):
        # Appending actions
        for action in self.acts_items:
            self.menu.remove(action["menu_item"])
            action["menu_item"].destroy()

        self.acts_items = []
        i = 0
        for action in self.act_manager.get_actions_list():
            menu_item = gtk.ImageMenuItem(action["name"])
            
            # Set icon
            action_dir = os.path.join(MSD.actions_data_dir, action["codename"])
            icon_path = os.path.join(action_dir,  action["codename"] + "_16x16.png")
            icon = gtk.Image()
            icon.set_from_file(icon_path)
            menu_item.set_image(icon)
            
            self.acts_items.append({"codename" : action["codename"],
                                    "menu_item" : menu_item,
                                    })

            # Signal
            menu_item.connect("activate", self.__action_activate_cb, action["codename"])
            
            menu_item.show()
            self.menu.insert (menu_item, i)
            i = i + 1
