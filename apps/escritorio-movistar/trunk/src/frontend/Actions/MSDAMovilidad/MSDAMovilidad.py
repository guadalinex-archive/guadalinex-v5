#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#           Cesar Garcia <tapia@openshine.com>
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
import os
import gtk
import gtk.glade
import gobject
import subprocess

from MobileManager.MobileManagerDbus import MOBILE_MANAGER_CONTROLLER_PATH,MOBILE_MANAGER_CONTROLLER_URI,MOBILE_MANAGER_CONTROLLER_INTERFACE_URI,MOBILE_MANAGER_DEVICE_PATH,MOBILE_MANAGER_DEVICE_URI,MOBILE_MANAGER_DEVICE_INFO_INTERFACE_URI,MOBILE_MANAGER_DEVICE_AUTH_INTERFACE_URI,MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI,MOBILE_MANAGER_DEVICE_XZONE_INTERFACE_URI
from MobileManager import CARD_STATUS_READY
from MSD.MSDProgressWindow import *

class MSDAMovilidad(MSD.MSDAction):
    def __init__(self, act_manager, conf, conn_manager):
        print "Init MSDAMovilidad"

	#Direccion del enlace
	self.link_url = "http://www.telefonicaonline.com/zonawifi"

        MSD.MSDAction.__init__(self, act_manager, conf, conn_manager)
        self.mcontroller = conn_manager.mcontroller
        
        self.security_manager = MSD.MSDSecurityManager(conf)
        
        action_dir = os.path.join(MSD.actions_data_dir , self.codename)

        self.az_xml = gtk.glade.XML(os.path.join(action_dir,"%s.glade" % self.codename),
                                    root="adsl_zone_window")

        self.adsl_zone_window = self.az_xml.get_widget("adsl_zone_window")
        self.az_banner_image = self.az_xml.get_widget("az_banner_image")
        action_dir = os.path.join(MSD.actions_data_dir , self.codename)
        self.az_banner_image.set_from_file(action_dir + "/movilidad_banner.png")
        self.az_cover_button = self.az_xml.get_widget("az_cover_button")
        self.az_key_button = self.az_xml.get_widget("az_key_button")
        self.az_close_button = self.az_xml.get_widget("az_close_button")

        self.adsl_zone_window.set_icon_from_file (os.path.join(action_dir, "MSDAMovilidad_16x16.png"))

        self.az_cover_button.connect("clicked", self.__az_cover_button_cb, None)
        
        self.cz_xml = gtk.glade.XML(os.path.join(action_dir,"%s.glade" % self.codename),
                                    root="cover_zone_window")
        self.cover_zone_window = self.cz_xml.get_widget("cover_zone_window")
	self.cz_link_button = self.cz_xml.get_widget("cz_link_button")
        self.cz_prov_combobox = self.cz_xml.get_widget("cz_prov_combobox")
        self.cz_local_combobox = self.cz_xml.get_widget("cz_local_combobox")
        self.cz_type_combobox = self.cz_xml.get_widget("cz_type_combobox")
        self.cz_hotspots_treview = self.cz_xml.get_widget("cz_hotspots_treview")
        self.cz_ok_button = self.cz_xml.get_widget("cz_ok_button")

        self.cover_zone_window.set_icon_from_file (os.path.join(action_dir, "MSDAMovilidad_16x16.png"))

	self.cz_link_button.connect("clicked", self.__cz_link_button_cb)
	self.cz_link_button.set_property("can-focus", False)

        self.czliststore = gtk.ListStore(str, str, str, str)
        self.modelfilter = self.czliststore.filter_new()
        self.modelfilter.set_visible_func(self.__visible_func_hotspots_treeview, None)
        
        self.cz_prov = []
        self.cz_local = []
        self.cz_prov_local = {}
        self.cz_type = []

        self.cz_prov_all = "Todas las provincias"
        self.cz_local_all = "Todas las localidades"
        self.cz_type_all = "Todos los tipos"
        
        for name in ["Name"] :
            column = gtk.TreeViewColumn(name,
                                        gtk.CellRendererText(),
                                        markup=0)
            self.cz_hotspots_treview.append_column(column)

        self.cz_hotspots_treview.set_model(self.modelfilter)
        
        self.__init_hotspots()

        self.cz_prov_ls = gtk.ListStore(str)
        cell = gtk.CellRendererText()
        self.cz_prov_combobox.pack_start(cell, True)
        self.cz_prov_combobox.add_attribute(cell, 'text', 0)
        self.cz_prov_combobox.set_model(self.cz_prov_ls)
        
        self.cz_local_ls = gtk.ListStore(str)
        cell = gtk.CellRendererText()
        self.cz_local_combobox.pack_start(cell, True)
        self.cz_local_combobox.add_attribute(cell, 'text', 0)
        self.cz_local_combobox.set_model(self.cz_local_ls)
        
        self.cz_type_ls = gtk.ListStore(str)
        cell = gtk.CellRendererText()
        self.cz_type_combobox.pack_start(cell, True)
        self.cz_type_combobox.add_attribute(cell, 'text', 0)
        self.cz_type_combobox.set_model(self.cz_type_ls)
        
        
        self.__init_prov_combobox()
        self.__init_local_combobox()
        self.__init_type_combobox()
        
        self.modelfilter.set_visible_func(self.__visible_func_hotspots_treeview, None)
        self.modelfilter.refilter()

        self.cz_prov_cb_h = self.cz_prov_combobox.connect("changed", self.__cz_prov_combobox_changed_cb, None)
        self.cz_local_cb_h = self.cz_local_combobox.connect("changed", self.__cz_local_combobox_changed_cb, None)
        self.cz_type_cb_h = self.cz_type_combobox.connect("changed", self.__cz_type_combobox_changed_cb, None)

        self.az_key_button.connect("clicked", self.__az_key_button_cb, None)
        self.mcontroller.connect ("active-device-changed", self.__active_device_changed_cb)
        self.mcontroller.connect ("active-dev-card-status-changed", self.__active_device_card_status_changed_cb)

    def __init_hotspots(self):
        import csv
        
        wifi_file = os.path.join(MSD.actions_data_dir , self.codename, "wifi.csv")
        reader = csv.reader(file(wifi_file), delimiter=";")
        for row in reader :
            name = "<b>%s</b>\n<small>%s</small>\n<small>%s - %s - %s</small>" % (row[1], row[2], row[4], row[3], row[5])
            self.czliststore.append([name, row[4], row[3],row[6]])
            if row[4] not in self.cz_prov :
                self.cz_prov.append(row[4])
                self.cz_prov_local[row[4]] = []
                
            if row[3] not in self.cz_local :
                if row[3] != "" :
                    self.cz_local.append(row[3])
                    tmp = self.cz_prov_local[row[4]]
                    if row[3] not in tmp :
                        tmp.append(row[3])
                    self.cz_prov_local[row[4]] = tmp
            
            if row[6] not in self.cz_type :
                if row[6] != "" :
                    self.cz_type.append(row[6])

        self.cz_prov.sort()
        self.cz_local.sort()
        self.cz_type.sort()

        for key in self.cz_prov_local.keys():
            tmp = self.cz_prov_local[key]
            tmp.sort()
            self.cz_prov_local[key] = tmp

    def __init_prov_combobox(self):
        print "__init_prov"
        self.cz_prov_ls.append([self.cz_prov_all])
        for x in self.cz_prov :
            self.cz_prov_ls.append([x])
        self.cz_prov_combobox.set_active(0)
    
    def __init_local_combobox(self):
        print "__init_local"
        self.cz_local_ls.append([self.cz_local_all])
        for x in self.cz_local :
            self.cz_local_ls.append([x])
        self.cz_local_combobox.set_active(0)
    
    def __init_type_combobox(self):
        print "__init_type"
        self.cz_type_ls.append([self.cz_type_all])
        for x in self.cz_type :
            self.cz_type_ls.append([x])
        self.cz_type_combobox.set_active(0)


    def __visible_func_hotspots_treeview(self, model, iter, data):
        ret = [False, False, False]
        
        prov_row = model.get_value(iter, 1)
        local_row = model.get_value(iter, 2)
        type_row = model.get_value(iter, 3)

        prov_cbox = self.cz_prov_combobox.get_active_text()
        local_cbox = self.cz_local_combobox.get_active_text()
        type_cbox = self.cz_type_combobox.get_active_text()

        if prov_cbox == self.cz_prov_all :
            ret[0]=True
        else:
            if prov_row == prov_cbox :
                ret[0]=True
            else:
                ret[0]=False

        if local_cbox == self.cz_local_all :
            ret[1]=True
        else:
            if local_row == local_cbox :
                ret[1]=True
            else:
                ret[1]=False
                
        if type_cbox == self.cz_type_all :
            ret[2]=True
        else:
            if type_row == type_cbox :
                ret[2]=True
            else:
                ret[2]=False

        if ret == [True, True, True] :
            return True
        else:
            return False

    def __active_device_changed_cb(self, mcontroller, udi):
        dev = self.mcontroller.get_active_device()
        
        if dev == None:
            self.az_key_button.set_sensitive(False)
        else:
            odev = self.mcontroller.get_device_obj_from_path(dev)
            if odev.has_capability(MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI) :
                if odev.get_card_status() == CARD_STATUS_READY :
                    self.az_key_button.set_sensitive(True)
                else:
                    self.az_key_button.set_sensitive(False)
            else:
                self.az_key_button.set_sensitive(False)

    def __active_device_card_status_changed_cb(self, mcontroller, status):
        dev = self.mcontroller.get_active_device()
        if dev == None:
            self.az_key_button.set_sensitive(False)
        else:
            odev = self.mcontroller.get_device_obj_from_path(dev)
            if odev.has_capability(MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI) :
                if status == CARD_STATUS_READY :
                    self.az_key_button.set_sensitive(True)
                else:
                    self.az_key_button.set_sensitive(False)
            else:
                self.az_key_button.set_sensitive(False)

    def __az_key_button_cb (self, widget, data):
        self.progress = MSDProgressWindow()
	self.progress.set_show_buttons(False)
	self.progress.show(_(u"Solicitando clave..."), _(u"Espere un momento, por favor..."))

        mainloop =  gobject.MainLoop(is_running=True)
        context = mainloop.get_context()

        while context.pending() :
            context.iteration()

        dev = self.mcontroller.get_active_device()
        cover_key = ""
        
        if dev != None :
            odev = self.mcontroller.get_device_obj_from_path(dev)
            if odev.has_capability(MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI) :
                cover_key = odev.get_cover_key(self.__cover_key_func,
                                               self.__cover_key_error_func)

    def __cover_key_func(self, response):
        self.progress.hide()
        self.progress.progress_dialog.destroy()
        
        action_dir = os.path.join(MSD.actions_data_dir , self.codename)
        dlg = gtk.MessageDialog(type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK)
        dlg.set_icon_from_file (os.path.join(action_dir, "MSDAMovilidad_16x16.png"))
        dlg.set_title (_(u'Envío de mensaje'))
        dlg.set_markup("<b>Respuesta recibida:</b>")
        dlg.format_secondary_markup("'%s'" % response)
        
        dlg.run()
        dlg.destroy()

    def __cover_key_error_func(self, e):
        self.progress.hide()
        self.progress.progress_dialog.destroy()

        action_dir = os.path.join(MSD.actions_data_dir , self.codename)
        dlg = gtk.MessageDialog(type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK)
        dlg.set_icon_from_file (os.path.join(action_dir, "MSDAMovilidad_16x16.png"))
        dlg.set_title (_(u'Envío de mensaje'))
        dlg.set_markup("<b>Respuesta recibida:</b>")
        dlg.format_secondary_markup("'%s'" % "Servicio no disponible")
        
        dlg.run()
        dlg.destroy()
    
    def __az_cover_button_cb (self, widget, data):
        self.cover_zone_window.show_all()
        self.cover_zone_window.run()
        self.cover_zone_window.hide()

    def __cz_prov_combobox_changed_cb(self, widget, data):
        print "__prov_combobox_changed"
        self.cz_local_combobox.handler_block(self.cz_local_cb_h)
        
        prov = self.cz_prov_combobox.get_active_text()
        self.cz_local_ls.clear()
        
        if prov == self.cz_prov_all :
            print "prov == all (%s)" % prov
            self.cz_local_ls.append([self.cz_local_all])
            for x in self.cz_local :
                self.cz_local_ls.append([x])
        else:
            print "prov != all (%s)" % prov
            self.cz_local_ls.append([self.cz_local_all])
            for x in self.cz_prov_local[prov] :
                self.cz_local_ls.append([x])

        self.cz_local_combobox.set_active(0)
        
        self.cz_local_combobox.handler_unblock(self.cz_local_cb_h)
        
        self.modelfilter.refilter()


    def __cz_local_combobox_changed_cb(self, widget, data):
        print "__local_combobox_changed"
        self.cz_prov_combobox.handler_block(self.cz_prov_cb_h)
        self.cz_local_combobox.handler_block(self.cz_local_cb_h)
        
        local = self.cz_local_combobox.get_active_text()
        prov = ""
        
        if local != self.cz_local_all :
            print "local != all (%s)" % local
            for key in self.cz_prov_local.keys():
                if local in self.cz_prov_local[key] :
                    prov = key
            
            self.cz_prov_ls.clear()
            self.cz_prov_ls.append([self.cz_prov_all])
            i = 1
            for x in self.cz_prov :
                self.cz_prov_ls.append([x])
                if x != prov :
                    i = i+1
                else:
                    self.cz_prov_combobox.set_active(i)
        else:
            print "local != all (%s)" % local

        self.cz_prov_combobox.handler_unblock(self.cz_prov_cb_h)
        self.cz_local_combobox.handler_unblock(self.cz_local_cb_h)

        self.modelfilter.refilter()

    def __cz_type_combobox_changed_cb(self, widget, data):
        self.modelfilter.refilter()

    def __cz_link_button_cb (self, widget):
        cmd = ["gnome-open", self.link_url]

        run = subprocess.Popen(cmd)
	returncode = None
	while(returncode is None):
	    gobject.main_context_default ().iteration ()
	    returncode = run.poll()
	
	widget.set_property("has-focus", False)

    def get_default_conf (self):
        return {'name' : _(u"Zonas WiFi de Telefónica"),
                'id' : 30,
                'tooltip' : _(u"Gestiona el acceso a la Zona WiFi de tu Tarifa Banda Ancha Móvil con WiFi"),
                'url' : "http://www.correomovil.movistar.es",
                'help_url' : 'em_50.htm#emadslwifi',
                'connection' : None,
                'connection_mandatory' : False
                 } 

    def launch_action(self):
        import os

        self.adsl_zone_window.show_all()
        ret = self.adsl_zone_window.run()
        self.adsl_zone_window.hide()
        
    def show_all(self):
        MSD.MSDAction.show_all(self)
        self.connection_vbox.hide()
        
