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

import MSD
import gtk
import gtk.glade
import os
import gobject
import MobileManager

class MSDAction:
    def __init__(self, act_manager, conf, conn_manager):
        print "Init MSDAction"
        MSD.init_i18n()
        self.act_manager = act_manager
        self.conf = conf
        self.conn_manager = conn_manager

        self.codename = str(self.__class__).split(".")[-1]
        action_dir = os.path.join(MSD.actions_data_dir , self.codename)
 
        self.xml_prefs = gtk.glade.XML(os.path.join(action_dir,"%s.glade" % self.codename),
                                       root="action_prefs_main_container")
        self.prefs_main_container = self.xml_prefs.get_widget ("action_prefs_main_container")
        self.conf_container = self.xml_prefs.get_widget("action_conf_container")

        self.xml_connect = gtk.glade.XML(os.path.join(MSD.glade_files_dir,"connect-preferences-frame.glade"), root="connect_container")
        self.connect_container = self.xml_connect.get_widget("connect_container")
        self.manual_connection_check_button = self.xml_connect.get_widget("manual_connection_check_button")
        self.connections_combobox = self.xml_connect.get_widget("connections_combobox")
        self.remove_service_button = self.xml_connect.get_widget("remove_service_button")
        self.install_service_alignment = self.xml_connect.get_widget("install_service_alignment")
        self.uninstall_service_alignment = self.xml_connect.get_widget("uninstall_service_alignment")
        self.install_service_button = self.xml_connect.get_widget("install_service_button")
        self.installed_service_label = self.xml_prefs.get_widget("installed_service_label")
        self.uninstalled_service_label = self.xml_prefs.get_widget("uninstalled_service_label")
        self.connection_vbox = self.xml_connect.get_widget("connection_vbox")
        
        if self.conf.exists_action_conf(self.codename) == False:
            default_conf = self.get_default_conf()
            default_conf['installed'] = True
            self.conf.set_default_action_conf(self.codename, default_conf)
        
        self.prefs_main_container.pack_end(self.connect_container)
        action_main_title = self.get_action_main_title()
        self.prefs_main_container.pack_start(action_main_title, expand=False)
        self.prefs_main_container.reorder_child(action_main_title, 0)

        self.progress_dialog = MSD.MSDProgressWindow()
        self.progress_dialog.set_show_buttons(False)
        #self.progress_dialog.set_image(gtk.STOCK_DIALOG_INFO)
        self.timer_id = 0

    def connect_signals(self):
        if self.get_conf_key_value("connection") == None:
            self.manual_connection_check_button.set_active(False)
            self.connections_combobox.set_active(0)
            self.connections_combobox.set_sensitive(False)
        else:
            self.manual_connection_check_button.set_active(True)
            self.connections_combobox.set_sensitive(True)
            connection_name = self.get_conf_key_value("connection")
            model = self.connections_combobox.get_model()
            tmp_iter = model.get_iter(0)
            self.connections_combobox.set_active(0)
            while tmp_iter != None:
                value = model.get_value(tmp_iter, 1)
                if value == connection_name :
                    self.connections_combobox.set_active_iter(tmp_iter)
                    break
                tmp_iter = model.iter_next(tmp_iter)
                
        self.manual_connection_check_button.connect("toggled", self.__toggle_connections_check_button_cb, None)
        self.connections_combobox.connect("changed", self.__connections_combobox_cb, None)
        
        self.remove_service_button.connect("clicked", self.__remove_service_button_cb, None)
        self.install_service_button.connect("clicked", self.__install_service_button_cb, None)

    def show_all(self):
        if self.get_conf_key_value ("installed") == True:
            self.conf_container.show()
            self.connection_vbox.show()
            self.install_service_alignment.hide()
            self.uninstall_service_alignment.show()
            self.installed_service_label.show()
            self.uninstalled_service_label.hide()
            if self.get_conf_key_value("connection") == None:
                self.manual_connection_check_button.set_active(False)
                self.connections_combobox.set_sensitive(False)
            else:
                self.manual_connection_check_button.set_active(True)
                self.connections_combobox.set_sensitive(True)
                
        else:            
            self.conf_container.hide()
            self.connection_vbox.hide()
            self.install_service_alignment.show()
            self.uninstall_service_alignment.hide()
            self.installed_service_label.hide()
            self.uninstalled_service_label.show()

    def __toggle_connections_check_button_cb (self, widget, data):
        if self.manual_connection_check_button.get_active() == False:
            self.connections_combobox.set_sensitive(False)
            self.set_conf_key_value("connection", None)
        else:
            self.connections_combobox.set_sensitive(True)
            model = self.connections_combobox.get_model()
            index = self.connections_combobox.get_active()
            try:
                tmp_iter = model.get_iter(index)
            except:
                self.connections_combobox.set_active(0)
                tmp_iter = model.get_iter(0)
            connection = model.get_value(tmp_iter, 1)
            print "action conn : %s" % connection
            self.set_conf_key_value("connection", connection)

    def __connections_combobox_cb (self, widget, data):
        model = self.connections_combobox.get_model()
        iter = self.connections_combobox.get_active_iter()
        self.set_conf_key_value("connection", model.get_value(iter, 1))
        print "action conn : %s " %  model.get_value(iter, 1)

    def __progress_timer_cb (self):
        self.progress_dialog.hide()

    def __remove_service_button_cb (self, widget, data):
        dlg = gtk.MessageDialog(type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK_CANCEL)
        dlg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        dlg.set_title(_(u"Desinstalar servicio"))
        dlg.set_markup(_(u"<b>¿Deseas desinstalar el servicio '%s'?</b>") % self.get_visible_action_name())
        dlg.format_secondary_markup(_(u"Podrás volver a instalarlo desde esta ventana de configuración."))
        ret = dlg.run()

        if ret == gtk.RESPONSE_OK :
            dlg.destroy()
            self.progress_dialog.show(_(u"Desinstalando servicio"), _(u"Por favor, espera mientras se desinstala el servicio '%s'.") % self.get_conf_key_value("name"))
            self.timer_id = gobject.timeout_add(2000, self.__progress_timer_cb)
            self.conf_container.hide()
            self.connection_vbox.hide()
            self.install_service_alignment.show()
            self.uninstall_service_alignment.hide()
            self.set_conf_key_value ("installed", False)
            self.act_manager.reload_actions_model(clear=True)
        else:
            dlg.destroy()

        self.remove_actions()

        self.show_all()

    def remove_actions(self):
        return

    def __install_service_button_cb(self, widget, data):
        self.progress_dialog.show(_(u"Instalando servicio"), _(u"Por favor, espera mientras se instala el servicio '%s'.") % self.get_conf_key_value("name"))
        self.timer_id = gobject.timeout_add(2000, self.__progress_timer_cb)
        self.conf_container.show()
        self.connection_vbox.show()
        self.install_service_alignment.hide()
        self.uninstall_service_alignment.show()
        self.set_conf_key_value ("installed", True)
        self.act_manager.reload_actions_model(clear=True)

        self.install_actions()

        self.show_all()

    def install_actions(self):
        return
        
    def get_preferences_treeview_row(self):
        action_dir = os.path.join(MSD.actions_data_dir , self.codename)
        icon_path = os.path.join(action_dir , self.codename + "_24x24.png")
        icon = gtk.gdk.pixbuf_new_from_file (icon_path)
        return [icon, self.get_visible_action_name()]

    def get_main_treeview_row(self):
        installed = self.get_conf_key_value("installed")
        if installed == True:
            icon = self.get_main_treeview_icon()
            color = False
        else: 
            icon = self.get_main_treeview_icon(uninstall=True)
            color = True
        return [icon, self.get_visible_action_name(), self.codename, color, self.get_conf_key_value("tooltip")]

    def get_main_treeview_icon(self, uninstall=False):
        action_dir = os.path.join(MSD.actions_data_dir , self.codename)
        if uninstall == False:
            icon_path = os.path.join(action_dir , self.codename + "_32x32.png")
        else:
            icon_path = os.path.join(action_dir , self.codename + "_uninstalled_32x32.png")
        return gtk.gdk.pixbuf_new_from_file (icon_path)

    def get_dialog_icon(self):
        action_dir = os.path.join(MSD.actions_data_dir , self.codename)
        icon_path = os.path.join(action_dir , self.codename + "_16x16.png")
        return gtk.gdk.pixbuf_new_from_file (icon_path)
    

    def get_action_main_title(self):
        hbox = gtk.HBox(spacing=10)
        action_dir = os.path.join(MSD.actions_data_dir , self.codename)
        icon_path = os.path.join(action_dir , self.codename + "_32x32.png")
        icon = gtk.Image()
        icon.set_from_file(icon_path)
        label = gtk.Label()
        label.set_markup("<b><big>%s</big></b>" % self.get_conf_key_value("name"))
        hbox.pack_start(icon, expand=False)
        hbox.pack_start(label, expand=False )
        
        return hbox

    def get_action_conf_widget(self):
        return self.prefs_main_container

    def set_connections_model(self, model):
        cell = gtk.CellRendererText()
        self.connections_combobox.pack_start(cell, True)
        self.connections_combobox.add_attribute(cell, 'text', 1)
        self.connections_combobox.set_model(model)

    def get_default_conf(self):
        return {'name' : "No hay nombre" ,
                'tooltip' : "No hay tooltip",
                'connection_mandatory' : True
                }

    def get_conf_key_value(self, key):
        if key == "name" or key == "tooltip" :
            default_conf = self.get_default_conf()
            return default_conf[key]
    
        return self.conf.get_action_key_value(self.codename, key)

    def set_conf_key_value(self, key, value):
        self.conf.set_action_key_value(self.codename, key, value)
        self.conf.save_conf()

    def get_visible_action_name (self):
        return self.get_conf_key_value("name")
        
    def get_prefs_widget(self, name):
        return self.xml_prefs.get_widget(name)

    def get_connection(self):
        conn_name = self.get_conf_key_value("connection")
        # Es conexion por defecto?
        if conn_name is None :            
            conn_name =  self.conf.get_default_connection_name ()

        return self.conn_manager.get_connection_info_dict(conn_name)
            
    def get_stats_id(self):
        return self.get_conf_key_value("id")
    
    def launch(self):
        if not self.get_conf_key_value("connection_mandatory") :
            self.launch_action()
            return
            
        if self.manual_connection_check_button.get_active() == False :
            #FIXED : Dialer
            if self.conn_manager.ppp_manager.status() == MobileManager.PPP_STATUS_CONNECTED:
                self.launch_action()
            else:
                conn_name = self.get_conf_key_value("connection")
                if self.conn_manager.connect_to_connection(connection_name=conn_name, action=self) == False:
                    self.conn_manager.error_on_connection()
        else:
            conn_name = self.get_conf_key_value("connection")
            if self.conn_manager.connect_to_connection(connection_name=conn_name, action=self) == False:
                self.conn_manager.error_on_connection()

    def launch_action (self):
        ret = os.system("gnome-open %s " % self.get_conf_key_value("url"))
        if ret != 0:
            if os.path.exists(self.bookmark_url.replace("file://","")):
                os.spawnle(os.P_NOWAIT, "%s" % self.bookmark_url.replace("file://",""), "", os.environ)
    

    def launch_help (self):
        dir_name = os.path.dirname(MSD.help_uri)
        help_file = os.path.join(dir_name, self.get_conf_key_value("help_url"))
        ret = os.popen("gconftool-2 -g /desktop/gnome/applications/browser/exec")
        url_cmd = ret.readline().split()
        if len(url_cmd) > 0 :
            os.system("%s 'file://%s' &" % (url_cmd[0], help_file))

