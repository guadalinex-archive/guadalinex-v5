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

class MSDServicesTab:
    def __init__(self, prefs_obj):
        self.xml = prefs_obj.xml
        self.prefs_obj = prefs_obj
        self.conf = prefs_obj.conf
        self.act_manager = prefs_obj.act_manager
        self.main_actions_view = prefs_obj.main_actions_view
        self.prefs_main_treeview = prefs_obj.prefs_main_treeview
        
        self.actions_treeview = self.xml.get_widget("actions_treeview")
        self.up_act_button = self.xml.get_widget("up_actions_treeview_button")
        self.down_act_button = self.xml.get_widget("down_actions_treeview_button")
        self.reset_act_button = self.xml.get_widget("restore_actions_treeview_button")
        self.hide_services_checkbutton = self.xml.get_widget("hide_services_checkbutton")

        self.services_main_image = self.xml.get_widget("services_main_image")
        self.services_main_image.set_from_file(os.path.join(MSD.icons_files_dir,"services_32x32.png"))

        #Populate services treview
        liststore = self.act_manager.get_actions_model()
        cellrenderpixbuf = gtk.CellRendererPixbuf()
        cellrenderpixbuf.set_property('cell-background', '#DEDEDE')
        cellrendertext = gtk.CellRendererText()
        cellrendertext.set_property('cell-background', '#DEDEDE')
        
        column = gtk.TreeViewColumn('Action_icon',
                                    cellrenderpixbuf,
                                    pixbuf=0, cell_background_set=3)
        self.actions_treeview.append_column(column)
        column = gtk.TreeViewColumn('Action_name',
                                    cellrendertext,
                                    markup=1, cell_background_set=3)
        self.actions_treeview.append_column(column)

        self.actions_treeview.set_model(liststore)

        self.actions_treeview.get_selection().set_mode(gtk.SELECTION_SINGLE)
        self.actions_treeview.get_selection().select_path(0)
        self.__changed_selection_actions_treeview_cb(self.actions_treeview.get_selection(),
                                                     None)

        #hide actions
        self.hide_services_checkbutton.set_active(self.conf.get_hide_uninstalled_services())

    def connect_signals(self):
        self.up_act_button.connect("clicked", self.__up_act_button_cb, None)
        self.down_act_button.connect("clicked", self.__down_act_button_cb, None)
        self.actions_treeview.get_selection().connect("changed", self.__changed_selection_actions_treeview_cb, None)
        self.hide_services_checkbutton.connect("toggled", self.__hide_services_toggled_cb, self.main_actions_view)
        self.reset_act_button.connect("clicked", self.__reset_services_button, None)

    def __reset_services_button(self, widget, data):
        self.conf.set_original_actions_order()
        if self.hide_services_checkbutton.get_active() == False:
            self.hide_services_checkbutton.set_active(True)
        self.conf.set_hide_uninstalled_services(True)
        self.conf.save_conf()
        
        self.act_manager.reload_actions_model(clear=True)
        #self.prefs_obj.reset_actions()
        self.up_act_button.set_sensitive(False)
        self.down_act_button.set_sensitive(False)
        
    def __hide_services_toggled_cb(self, checkbutton, main_actions_view):
        self.conf.set_hide_uninstalled_services (checkbutton.get_active())
        self.act_manager.reload_actions_model(clear=True)
        self.__changed_selection_actions_treeview_cb(self.actions_treeview.get_selection(), None)
    
    def __changed_selection_actions_treeview_cb(self, selection, data):
        installed_actions = 0
        actions = self.conf.get_actions_order()
        for x in actions:
            if self.conf.get_action_key_value(x, "installed") == True:
                installed_actions = installed_actions + 1

        if selection.path_is_selected(0) == True:
            self.up_act_button.set_sensitive(False)
            self.down_act_button.set_sensitive(True)
            return
        else: 
            if installed_actions == 0 and self.conf.get_hide_uninstalled_services() == True:
                self.up_act_button.set_sensitive(False)
                self.down_act_button.set_sensitive(False)
                return
            
            if self.conf.get_hide_uninstalled_services() == False:
                last_pos = len(self.conf.get_actions_order())-1
            else:
                last_pos = installed_actions - 1
                
            if selection.path_is_selected(last_pos) == True:
                self.up_act_button.set_sensitive(True)
                self.down_act_button.set_sensitive(False)
            else:
                (model, iter) = selection.get_selected()
                if iter == None:
                    self.up_act_button.set_sensitive(False)
                    self.down_act_button.set_sensitive(False)
                else:
                    self.up_act_button.set_sensitive(True)
                    self.down_act_button.set_sensitive(True)


    def __up_act_button_cb(self, widget, data):
        selection = self.actions_treeview.get_selection()
        (model, iter) = selection.get_selected()
        codename = model.get_value(iter, 2)
        actions = self.conf.get_actions_order()
        orig_pos = actions.index(codename)
        
        new_pos = self.act_manager.up_action(codename)
        self.act_manager.reload_actions_model()
        selection.select_path(new_pos)
        self.__changed_selection_actions_treeview_cb(self.actions_treeview.get_selection(), None)
        #self.prefs_obj.move_up_action_in_prefs_treeview(orig_pos, new_pos)
        
    def __down_act_button_cb(self, widget, data):
        selection = self.actions_treeview.get_selection()
        (model, iter) = selection.get_selected()
        codename = model.get_value(iter, 2)
        actions = self.conf.get_actions_order()
        orig_pos = actions.index(codename)
        
        new_pos = self.act_manager.down_action(codename)
        self.act_manager.reload_actions_model()
        selection.select_path(new_pos)
        self.__changed_selection_actions_treeview_cb(self.actions_treeview.get_selection(), None)
        #self.prefs_obj.move_down_action_in_prefs_treeview(orig_pos, new_pos)

        

