#!/usr/bin/python
# -*- coding: iso-8859-15 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica Móviles España S.A.U.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

import gtk
import gtk.glade
import gobject
import MobileManager.ui
import os

class MobileATOptionsButton(gtk.Button) :
    def __init__(self, mcontroller):
        gtk.Button.__init__(self)
        
        self.mcontroller = mcontroller
        self.xml = gtk.glade.XML(os.path.join(MobileManager.ui.mobilemanager_glade_path, "mm_menu_at_options.glade"))
        
        self.menu = self.xml.get_widget("menu_pcmcia")
        self.gest_pin_branch = self.xml.get_widget("gest_pin_branch")
        self.gest_pin_branch_menu = self.xml.get_widget("gest_pin_branch_menu")
        self.activate_pin = self.xml.get_widget("activate_pin")
        self.deactivate_pin = self.xml.get_widget("deactivate_pin")
        self.change_pin = self.xml.get_widget("change_pin")
        self.select_tech = self.xml.get_widget("select_tech")
        self.select_tech_menu = self.xml.get_widget("select_tech_menu")
        self.auto_tech = self.xml.get_widget("auto_tech")
        self.only_utms = self.xml.get_widget("only_utms")
        self.preferred_utms = self.xml.get_widget("preferred_utms")
        self.only_gprs = self.xml.get_widget("only_gprs")
        self.preferred_gprs = self.xml.get_widget("preferred_gprs")
        self.select_operator = self.xml.get_widget("select_operator")
        self.select_operator_menu = self.xml.get_widget("select_operator_menu")
        self.auto_oper = self.xml.get_widget("auto_oper")
        self.manual_oper = self.xml.get_widget("manual_oper")
        self.activate_card = self.xml.get_widget("activate_card")
        self.deactivate_card = self.xml.get_widget("deactivate_card")
        
        dict = {"on_pin_activate" : self.__on_pin_activate,
                "on_pin_deactivate" : self.__on_pin_deactivate,
                "on_card_activate" : self.__on_card_activate,
                "on_card_deactivate" : self.__on_card_deactivate,
                "on_change_pin_activate" : self.__on_change_pin_activate,
                "on_auto_tech_activate" : self.__on_auto_tech_activate,
                "on_only_utms_activate" : self.__on_only_utms_activate,
                "on_preferred_utms_activate" : self.__on_preferred_utms_activate,
                "on_only_gprs_activate" : self.__on_only_gprs_activate,
                "on_preferred_gprs_activate" : self.__on_preferred_gprs_activate
                }  
        self.xml.signal_autoconnect(dict)
        

        self.manual_oper_hid = self.manual_oper.connect("toggled",  self.__on_manual_oper_activate)
        self.auto_oper_hid = self.auto_oper.connect("toggled",  self.__on_auto_oper_activate)
        
        mcontroller.connect("active-dev-pin-act-status-changed", self.__pin_activate_status_changed_cb, None)
        mcontroller.connect("active-dev-card-status-changed", self.__card_status_changed_cb, None)
        mcontroller.connect("active-device-changed", self.__active_device_changed_cb, None)
        mcontroller.dialer.connect("connected", self.__connected_cb)
        mcontroller.dialer.connect("disconnected", self.__disconnected_cb)
        
        self.connect("clicked", self.__show_menu_cb, None)

        dev = mcontroller.get_active_device()
        if dev == None :
            self.set_sensitive(False)
            return
        
        if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
            self.set_sensitive(False)
            return
        else:
            self.set_sensitive(True)
            self.__card_status_changed_cb(mcontroller, dev.get_card_status(), None)

    def __show_menu_cb(self, widget, data):
        self.popup(self)

    def __card_status_changed_cb(self, mcontroller, status, data):
        dev = self.mcontroller.get_active_device()
        if dev == None :
            self.set_sensitive(False)
            return
        if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
            self.set_sensitive(False)
            return
        
        self.set_sensitive(True)
        
        self.set_sensitive_all_options(False)

        if status == None :
            status = dev.get_card_status()
        
        if status == MobileManager.CARD_STATUS_READY or status == MobileManager.CARD_STATUS_ATTACHING:
            self.set_sensitive_tech_options(True)
            self.set_sensitive_operator_options(True)

        if status == MobileManager.CARD_STATUS_READY :
            if dev.is_pin_active():
                self.deactivate_pin.show()
                self.activate_pin.hide()
                self.set_sensitive_pin_options(True)
            else:
                self.deactivate_pin.hide()
                self.activate_pin.show()
                self.set_sensitive_pin_options(False)
                self.activate_pin.set_sensitive(True)

            self.auto_oper.handler_block(self.auto_oper_hid)
            self.manual_oper.handler_block(self.manual_oper_hid)
            if dev.is_carrier_auto() == True:
                self.auto_oper.set_active(True)
                self.manual_oper.set_active(False)
            else:
                self.auto_oper.set_active(False)
                self.manual_oper.set_active(True)
                
            self.auto_oper.handler_unblock(self.auto_oper_hid)
            self.manual_oper.handler_unblock(self.manual_oper_hid)
            
        else:
            self.deactivate_pin.hide()
            self.activate_pin.show()
            self.set_sensitive_pin_options(False)
            
        if dev.is_on() :
            self.activate_card.hide()
            self.deactivate_card.show()
        else:
            self.activate_card.show()
            self.deactivate_card.hide()

    def __active_device_changed_cb(self, mcontroller, udi, data):
        dev = mcontroller.get_active_device()
        if dev == None :
            self.set_sensitive(False)
            return
        
        if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
            self.set_sensitive(False)
            return
        else:
            self.set_sensitive(True)
            self.__card_status_changed_cb(mcontroller, dev.get_card_status(), None)

    def __pin_activate_status_changed_cb(self, mcontroller, status, data):
        dev = self.mcontroller.get_active_device()
        self.__card_status_changed_cb(self.mcontroller, dev.cached_status_values["card_status"], None)
        
    def popup(self, widget=None):
        dev = self.mcontroller.get_active_device()
        if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
            print "NO AT CAPABILITY"
            return False


        if widget == None :
            self.menu.popup(None, None, self.__widget_pos, 0, 0, data=self)
        else:
            self.menu.popup(None, None, self.__widget_pos, 0, 0, data=widget)
        
        self.menu.show()
        return True

    def set_active(self, value):
        self.menu_activated = value
        
    def set_sensitive_all_options(self, value):
        self.set_sensitive_tech_options(value)
        self.set_sensitive_pin_options(value)
        self.set_sensitive_operator_options(value)

    def set_sensitive_tech_options(self, value):
        self.auto_tech.set_sensitive(value)
        self.only_utms.set_sensitive(value)
        self.preferred_utms.set_sensitive(value)
        self.only_gprs.set_sensitive(value)
        self.preferred_gprs.set_sensitive(value)

    def set_sensitive_pin_options(self, value):
        self.activate_pin.set_sensitive(value)
        self.deactivate_pin.set_sensitive(value)
        self.change_pin.set_sensitive(value)

    def set_sensitive_operator_options(self, value):
        self.auto_oper.set_sensitive(value)
        self.manual_oper.set_sensitive(value)
    
    def __widget_pos(self, widget, data):
        widget_window = data.window
        x, y = widget_window.get_position()
        widget_rect = data.get_allocation()
        return (x+widget_rect.x, y+widget_rect.y+widget_rect.height, True)

    def __on_toogle_pin_activate (self, widget):
        dev = self.mcontroller.get_active_device()
        pin_status = dev.is_pin_active()
        if pin_status == None:
            return
        
        dialog = MobileManager.ui.MobileManagePinDialog(self.mcontroller)
        if pin_status == True:
            dialog.run_deactivate()
        else:
            dialog.run_activate()
        self.menu.hide()

    def __on_pin_activate (self, widget):
        dialog = MobileManager.ui.MobileManagePinDialog(self.mcontroller)
        dialog.run_activate()
        
        self.menu.hide()
        
    def __on_pin_deactivate (self, widget):
        dialog = MobileManager.ui.MobileManagePinDialog(self.mcontroller)
        dialog.run_deactivate()
        
        self.menu.hide()

    def __on_change_pin_activate (self, widget):
        dev = self.mcontroller.get_active_device()
        pin_status = dev.is_pin_active()
        if pin_status == None:
            return
        
        if pin_status == True:
            dialog = MobileManager.ui.MobileChangePinDialog(self.mcontroller)
            dialog.run()
        
        self.menu.hide()

    def __on_card_activate(self, widget):
        dev = self.mcontroller.get_active_device()
        if dev != None:
            dev.turn_on()

    def __on_card_deactivate(self, widget):
        dev = self.mcontroller.get_active_device()
        if dev != None:
            if self.mcontroller.dialer.status() != MobileManager.PPP_STATUS_DISCONNECTED :
                self.mcontroller.dialer.stop()
            dev.turn_off()
            
    def __on_auto_tech_activate (self, widget):
        self.menu.hide()
        if self.auto_tech.get_active() != True:
            return
        dev = self.mcontroller.get_active_device()
        dev.set_mode_domain(MobileManager.CARD_TECH_SELECTION_AUTO, MobileManager.CARD_DOMAIN_CS_PS)

    def __on_only_utms_activate (self, widget):
        self.menu.hide()
        if self.only_utms.get_active() != True:
            return
        dev = self.mcontroller.get_active_device()
        dev.set_mode_domain(MobileManager.CARD_TECH_SELECTION_UMTS, MobileManager.CARD_DOMAIN_CS_PS)

    def __on_preferred_utms_activate (self, widget):
        self.menu.hide()
        if self.preferred_utms.get_active() != True:
            return
        dev = self.mcontroller.get_active_device()
        dev.set_mode_domain(MobileManager.CARD_TECH_SELECTION_UMTS_PREFERED, MobileManager.CARD_DOMAIN_CS_PS)

    def __on_only_gprs_activate (self, widget):
        self.menu.hide()
        if self.only_gprs.get_active() != True:
            return
        dev = self.mcontroller.get_active_device()
        dev.set_mode_domain(MobileManager.CARD_TECH_SELECTION_GPRS, MobileManager.CARD_DOMAIN_CS_PS)

    def __on_preferred_gprs_activate (self, widget):
        self.menu.hide()
        if self.preferred_gprs.get_active() != True:
            return
        dev = self.mcontroller.get_active_device()
        dev.set_mode_domain(MobileManager.CARD_TECH_SELECTION_GRPS_PREFERED, MobileManager.CARD_DOMAIN_CS_PS)

    def __on_auto_oper_activate (self, widget):
        
        self.menu.hide()
        self.auto_oper.handler_block(self.auto_oper_hid)
        self.manual_oper.handler_block(self.manual_oper_hid)
        self.manual_oper.set_active(False)
        self.auto_oper.set_active(True)
        self.manual_oper.handler_unblock(self.manual_oper_hid)
        self.auto_oper.handler_unblock(self.auto_oper_hid)
        
        dev = self.mcontroller.get_active_device()
        dev.set_carrier_auto_selection()
            
    def __on_manual_oper_activate (self, widget):
        if self.manual_oper.get_active() == False :
            if self.auto_oper.get_active() != False :
                return
        
        self.menu.hide()
        dev = self.mcontroller.get_active_device()
        dialog = MobileManager.ui.MobileCarrierSelectorDialog(self.mcontroller)
        res = dialog.run()
        
        self.auto_oper.handler_block(self.auto_oper_hid)
        self.manual_oper.handler_block(self.manual_oper_hid)
        if not dev.is_attached():
            print "-------> Set radio to AUTO_OPER = True"
            dev.set_carrier_auto_selection()
            self.auto_oper.set_active(True)
            self.manual_oper.set_active(False)
        else:
            print "-------> Set radio to MANUAL_OPER = True"
            self.manual_oper.set_active(True)
            self.auto_oper.set_active(False)
        
        self.auto_oper.handler_unblock(self.auto_oper_hid)
        self.manual_oper.handler_unblock(self.manual_oper_hid)


    def __connected_cb (self, dialer):
        dev = self.mcontroller.get_active_device()
	if MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
            self.set_sensitive_all_options(False)
            self.deactivate_card.show()
            self.activate_card.hide()

    def __disconnected_cb (self, dialer):
        dev = self.mcontroller.get_active_device()
        if MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
            self.__card_status_changed_cb(self.mcontroller, None, None)

gobject.type_register(MobileATOptionsButton)
