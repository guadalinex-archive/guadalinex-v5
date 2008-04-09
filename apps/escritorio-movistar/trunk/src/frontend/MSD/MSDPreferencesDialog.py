#!/usr/bin/python
# -*- coding: iso-8859-15 -*-
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

class MSDPreferencesDialog:
    def __init__(self, main_window_obj):
        self.main_window_obj = main_window_obj
        self.conf = main_window_obj.MSDConf 
        self.act_manager = main_window_obj.MSDActManager
        self.main_actions_view = main_window_obj.main_actions_view
        self.main_device_label= main_window_obj.device_selected_label
        self.main_stats = main_window_obj.main_stats
        self.main_connmanager = main_window_obj.MSDConnManager
        self.exporter = MSD.MSDExporter(self.conf)
        self.importer = MSD.MSDImporter(self.conf)
        
        self.xml = gtk.glade.XML(MSD.glade_files_dir + "preferences.glade")
        self.preferences_dialog = self.xml.get_widget("preferences_dialog")
        self.preferences_dialog.set_icon_from_file(MSD.icons_files_dir + "configuracion_16x16.png")
        self.close_button = self.xml.get_widget("close_button")
        self.actions_prefs_area = self.xml.get_widget("actions_prefs_area")
        self.prefs_notebook = self.xml.get_widget("preferences_notebook")
        self.prefs_notebook.set_show_tabs(False)
        self.prefs_main_treeview = self.xml.get_widget("preferences_main_treeview")
        self.actions_iters = {}
        self.actions_paths = {}
        self.last_preference_path = None

        self.populate_prefs_main_treeview()


        self.initial_device_selected = self.conf.get_device_selected()

        #Init Tabs Objects
        #self.addressbook_tab = MSD.MSDAddressbookTab(self)
	self.connections_tab = MSD.MSDConnectionsTab(self)
        self.bookmarks_tab = MSD.MSDBookmarksTab(self)
        self.services_tab = MSD.MSDServicesTab(self)
        self.security_tab = MSD.MSDSecurityTab(self)
        self.devices_tab = MSD.MSDDevicesTab(self, self.main_window_obj.mcontroller)
        self.tabs=[self.bookmarks_tab,
                   self.services_tab,
                   self.security_tab,
                   self.devices_tab,
                   self.connections_tab]
        
        #Signals
        treeselection = self.prefs_main_treeview.get_selection()
        treeselection.set_mode(gtk.SELECTION_SINGLE)
        treeselection.connect("changed" , self.__prefs_main_treview_row_changed_cb, None)
        treeselection.select_path(0)

        self.preferences_dialog.connect("delete_event", self.__delete_event_preferences_cb)
        self.close_button.connect("clicked", self.__close_preferences_cb, None)

        #Connect the tab's signals
        self.bookmarks_tab.connect_signals()
        self.services_tab.connect_signals()
        self.connections_tab.connect_signals()
        self.security_tab.connect_signals()


    def populate_prefs_main_treeview(self):
        #Populate prefs main treeview
        treestore = self.prefs_main_treeview.get_model()
        treestore = gtk.TreeStore(gtk.gdk.Pixbuf, str, int)
        
        cell = gtk.CellRendererPixbuf()
        #cell.set_property('xpad', 10)
        
        column = gtk.TreeViewColumn('PMT_icon', cell,
                                    pixbuf=0)
        self.prefs_main_treeview.append_column(column)
        column = gtk.TreeViewColumn('PMT_name', gtk.CellRendererText(),
                                    markup=1)
        self.prefs_main_treeview.append_column(column)
        
        # The third column of the treeview represents the notebook page for
        # each section or action.
        
        i = 0
        for x in [ "Accesos directos","Servicios","Datos de usuario",
                  "Mis dispositivos","Conexiones"]:
            if x == "Agenda":
                icon_path = os.path.join(MSD.icons_files_dir, "addressbook_24x24.png")
            if x == "Accesos directos":
                icon_path = os.path.join(MSD.icons_files_dir, "bookmarks_24x24.png")
            if x == "Datos de usuario":
                icon_path = os.path.join(MSD.icons_files_dir, "security_24x24.png")
            if x == "Mis dispositivos":
                icon_path = os.path.join(MSD.icons_files_dir, "devices_24x24.png")
            if x == "Conexiones":
                icon_path = os.path.join(MSD.icons_files_dir, "connections_24x24.png")
            if x == "Servicios":
                icon_path = os.path.join(MSD.icons_files_dir, "services_24x24.png")
            
            icon = gtk.gdk.pixbuf_new_from_file (icon_path)
            piter = treestore.append(None, [icon, '%s' % x] + [i])
            i = i + 1
            if x == "Servicios" :
                self.services_piter = piter

        acts_list = self.act_manager.get_actions_list(original_order=True)
        for action in acts_list:
            piter = treestore.append(self.services_piter,
                                     action["prefs_treeview_row"] + [i])
            self.actions_iters[action["codename"]] = piter
            self.actions_paths[action["codename"]] = treestore.get_path(piter)
            i = i + 1
        
        self.prefs_main_treeview.set_model(treestore)
        self.prefs_main_treeview.expand_all()

        #Populate actions in the treeview
        for action in acts_list :
            self.prefs_notebook.append_page(action["capplet"],
                                            gtk.Label("")) 

    def show_all(self, tab=None):
        self.preferences_dialog.show_all()
        self.prefs_main_treeview.collapse_row(1)
        treeselection = self.prefs_main_treeview.get_selection()

        if tab != None:
            if type(tab) == int:
                self.set_preferences_tab(tab)
            elif len(tab) > 1:
                self.prefs_main_treeview.expand_row(1, True)
                treeselection.select_path(tab)
        else:
            if self.last_preference_path != None:
                if len(self.last_preference_path) > 1:
                    self.prefs_main_treeview.expand_row(1, True)
                treeselection.select_path(self.last_preference_path)
            else:
                treeselection.select_path(0)

        self.__prefs_main_treview_row_changed_cb(treeselection, None)

        self.connections_tab.show_all()
        self.act_manager.show_actions_conf()

        self.initial_device_selected = self.conf.get_device_selected()

        self.close_button.grab_focus()

    def get_actions_path_from_codename(self, codename):
        return self.actions_paths[codename]

    def get_connections_model(self):
        return self.connections_tab.connections_treeview.get_model()

    def set_preferences_tab (self, tab_number):
        self.prefs_notebook.set_current_page(tab_number + 1)
        selection = self.prefs_main_treeview.get_selection()
        selection.select_path (tab_number)

    def reset_actions(self):
        model = self.prefs_main_treeview.get_model()
        actions = self.conf.get_actions_order()
        new_order = []

        for codename in actions :
            tmp_iter = self.actions_iters[codename]
            path = model.get_path(tmp_iter)
            new_order.append(path[1])

        model.reorder(self.services_piter, new_order)
            
            
    def move_up_action_in_prefs_treeview(self, orig_pos, final_pos):
        model = self.prefs_main_treeview.get_model()
        parent_path = model.get_path(self.services_piter)
        orig_iter = model.get_iter((parent_path[0], orig_pos))
        final_iter = model.get_iter((parent_path[0], final_pos))
        model.move_before(orig_iter, final_iter)

    def move_down_action_in_prefs_treeview(self, orig_pos, final_pos):
        model = self.prefs_main_treeview.get_model()
        parent_path = model.get_path(self.services_piter)
        orig_iter = model.get_iter((parent_path[0], orig_pos))
        final_iter = model.get_iter((parent_path[0], final_pos))
        model.move_after(orig_iter, final_iter)

## DEPRECATED
    def card_in_cb(self):
        print "MSDPreferencesDialog : card in"
        
    def card_out_cb(self):
        print "MSDPreferencesDialog : card out"

    def card_on_cb(self):
        print "MSDPreferencesDialog : card on"

    def card_on_no_sim_cb(self):
        print "MSDPreferencesDialog : card on no sim"
##--- DEPRECATED
        
    def __close_preferences_cb(self, widget, data):
        # validacion
        for t in self.tabs:
            if hasattr(t,"validate_tab_data"):
                if not t.validate_tab_data():
                    print "validation failed"
                    idx = self.tabs.index(t)
                    self.set_preferences_tab(idx)
                    return True
        try:
            treeselection = self.prefs_main_treeview.get_selection()
            (model, iter) = treeselection.get_selected()
            self.last_preference_path = model.get_path(iter)
        except:
            self.last_preference_path = None                    
        
        self.preferences_dialog.hide_all()
        self.conf.save_conf()
        
        return True
    
    def __delete_event_preferences_cb(self, widget, data):
        return self.__close_preferences_cb(widget, data)

    
    def __prefs_main_treview_row_changed_cb(self, selection, data):
        (model, pathlist) = selection.get_selected_rows()
        if len(pathlist) > 0:
            iter = model.get_iter(pathlist[0])
            value = model.get_value(iter, 2)
            self.prefs_notebook.set_current_page(value + 1)
        
