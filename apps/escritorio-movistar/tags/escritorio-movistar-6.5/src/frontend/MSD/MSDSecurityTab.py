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

class MSDSecurityTab:
    def __init__ (self, pref_obj):
        self.xml = pref_obj.xml
        self.conf = pref_obj.conf
        self.conf.add_observer(self)
        self.signal_handlers ={}
        #Security Tab
        self.s_auth_on_radio_button = self.xml.get_widget("security_auth_on_radio_button")
        self.s_auth_off_radio_button = self.xml.get_widget("security_auth_off_radio_button")
        self.s_ask_password_check_button = self.xml.get_widget("security_ask_password_check_button")
        self.s_cell_number_entry = self.xml.get_widget("security_cell_number_entry")
        self.s_cell_password_entry = self.xml.get_widget("security_cell_password_entry")
        self.s_auth_off_radio_button = self.xml.get_widget("security_auth_off_radio_button")
        self.security_main_image = self.xml.get_widget("security_main_image")
        self.security_main_image.set_from_file(os.path.join(MSD.icons_files_dir,"security_32x32.png"))

        #Complete the security form
        self.load_from_conf()

    
    def configuration_changed(self,conf):
        self.__block_signals()
        self.load_from_conf()
        self.__unblock_signals()
        pass
    
    def __block_signals(self):
        for key in self.signal_handlers.keys():
            widget = key
            handler_id = self.signal_handlers[key]
            widget.handler_block(handler_id)

    def __unblock_signals(self):
        for key in self.signal_handlers.keys():
            widget = key
            handler_id = self.signal_handlers[key]
            widget.handler_unblock(handler_id)
    
    def load_from_conf(self):        
        cell_info = self.conf.get_celular_info()
        if cell_info[0] != None:
            self.s_cell_number_entry.set_text(cell_info[0])
        if cell_info[1] != None:
            self.s_cell_password_entry.set_text(cell_info[1])

        if self.conf.get_ask_password_activate() == True:
            self.s_ask_password_check_button.set_active(True)
            self.s_cell_password_entry.set_sensitive(False)
        else:
            self.s_ask_password_check_button.set_active(False)
            self.s_cell_password_entry.set_sensitive(True)
            
        if self.conf.get_auth_activate() == True:
            self.s_auth_on_radio_button.set_active(True)        
        else:
            self.s_auth_off_radio_button.set_active(True)
            self.s_cell_password_entry.set_sensitive(False)
                        
     
        
    
    def connect_signals(self):
        self.signal_handlers = {}

        self.signal_handlers[self.s_auth_on_radio_button]= self.s_auth_on_radio_button.connect("toggled", self.__s_auth_on_radio_button_cb, None)
        
        self.signal_handlers[self.s_ask_password_check_button]= \
                                    self.s_ask_password_check_button.connect("toggled", self.__s_ask_password_check_button_cb, None)


        self.signal_handlers[self.s_cell_number_entry ]=\
                                    self.s_cell_number_entry.connect("changed", self.__s_cell_entry_changed_cb, None)


        self.signal_handlers[self.s_cell_password_entry ]=\
                                    self.s_cell_password_entry.connect("changed", self.__s_cell_entry_changed_cb, None)


        self.__s_auth_on_radio_button_cb(self.s_auth_on_radio_button, None)


    def __s_auth_on_radio_button_cb(self, widget, data):
        print "authn radio cb"        
        if self.s_auth_on_radio_button.get_active() == True: 
            self.s_cell_number_entry.set_sensitive(True)
            self.s_cell_password_entry.set_sensitive(True)
            self.s_ask_password_check_button.set_sensitive(True)
            self.conf.set_auth_activate(True)
        else:
            self.s_cell_number_entry.set_sensitive(False)
            self.s_cell_password_entry.set_sensitive(False)
            self.s_ask_password_check_button.set_sensitive(False)
            self.conf.set_auth_activate(False)
     
        self.conf.save_conf()

    def __s_ask_password_check_button_cb(self, widget, data):
        print "ask pass cb"
        if self.s_ask_password_check_button.get_active() == True:
            self.conf.set_ask_password_activate(True)
            self.s_cell_password_entry.set_sensitive(False)
        else:
            self.conf.set_ask_password_activate(False)
            self.s_cell_password_entry.set_sensitive(True)
        self.conf.save_conf()

    def __s_cell_entry_changed_cb(self, widget, data):
        if len (self.s_cell_number_entry.get_text()) > 0 :
            number = self.s_cell_number_entry.get_text()
        else :
            number = None
            
        if len (self.s_cell_password_entry.get_text()) > 0 :
            password = self.s_cell_password_entry.get_text()
        else :
            password = None

        self.conf.set_celular_info(number, password)
        self.conf.save_conf()



    def validate_tab_data(self):        
        phone = self.s_cell_number_entry.get_text()
        if len(phone) < 1 :
            return True
        try:
            int(phone)
            return True
        except Exception ,msg:
             dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
             dlg.set_icon_from_file(MSD.icons_files_dir + "security_24x24.png")
             dlg.set_markup(MSD.MSG_INVALID_PHONE_TITLE)
             dlg.format_secondary_markup(MSD.MSG_INVALID_PHONE)
             dlg.run()
             dlg.destroy()
             return False
