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

import os
import imp
import gtk
import gtk.glade
import MSD
import compileall

class MSDActionsManager:
    def __init__(self, conf, conn_manager):
        print "Init MSDActionsManager"
        self.action_objs_dict = {}
        self.conf = conf
        self.conn_manager = conn_manager
        self.liststore = None
        self.systray = None

        compileall.compile_dir(MSD.actions_py_dir, quiet=1)
        
        for action in os.listdir(MSD.actions_py_dir):
            if action == ".svn": #evita el directorio .svn
                continue

            module = self.__import_action_compiled(os.path.join(MSD.actions_py_dir , "%s/%s.pyc" % (action,action)), action)
            exec ("action_obj = module.%s(self, self.conf, self.conn_manager)" % action)
            self.action_objs_dict[action] = action_obj

        act_sorted = self.conf.get_actions_order()
        self.liststore = self.get_actions_model()

    def __import_action_compiled(self, filecode, name):
        module = imp.load_compiled(name, filecode)
        return module

    def set_systray(self, systray):
        self.systray = systray

    def get_actions_model(self, new=False):
        if self.liststore == None or new == True:
            if self.liststore == None:
                self.liststore = gtk.ListStore(gtk.gdk.Pixbuf, str, str, 'gboolean', str)
            else:
                self.liststore.clear()
                
            hide_uninstall = self.conf.get_hide_uninstalled_services()
            
            for x in self.conf.get_actions_order():
                obj = self.action_objs_dict[x]
                if not (obj.get_conf_key_value("installed") == False and hide_uninstall == True):
                    self.liststore.append(obj.get_main_treeview_row())
        
        return self.liststore

    def reload_actions_model(self, clear=False):

        # Reload System Tray Menu
        if self.systray != None:
            self.systray.reload_actions()
            self.systray.update_installed_actions_items()
        
        if clear == True:
            self.get_actions_model(new=True)
            return
        
        hide_uninstall = self.conf.get_hide_uninstalled_services()
        actions_iter_dict = {}

        tmp_iter = self.liststore.get_iter(0)
        while tmp_iter != None :
            actions_iter_dict[self.liststore.get_value(tmp_iter, 2)] = tmp_iter
            tmp_iter= self.liststore.iter_next(tmp_iter)

        actual_actions_list = self.conf.get_actions_order()

        new_order = []
        if hide_uninstall == False:
            for codename in actual_actions_list:
                tmp_iter = actions_iter_dict[codename]
                pos = self.liststore.get_path(tmp_iter)
                new_order.append(pos[0])
        else:
            for codename in actual_actions_list:
                if self.conf.get_action_key_value(codename, "installed") == True: 
                    tmp_iter = actions_iter_dict[codename]
                    pos = self.liststore.get_path(tmp_iter)
                    new_order.append(pos[0])

        if len(new_order) > 0 :
            self.liststore.reorder(new_order)

    def get_actions_list(self, original_order=False):
        acts_list = []
        if original_order == True:
            actions = self.conf.get_original_actions_order()
        else:
            actions = self.conf.get_actions_order()
        
        for x in actions:
            obj = self.action_objs_dict[x]
            acts_list.append({"codename" : x,
                              "name" : obj.get_visible_action_name(),
                              "prefs_treeview_row" : obj.get_preferences_treeview_row(),
                              "capplet" : obj.get_action_conf_widget(),
                              "installed" : obj.get_conf_key_value("installed")})

        return acts_list

    def up_action (self, codename):
        actions = self.conf.get_actions_order()
        index = actions.index(codename)
        hide_uninstall = self.conf.get_hide_uninstalled_services()

        list = range(index)
        list.reverse()
        
        for x in list:
            if hide_uninstall == True:
                obj = self.action_objs_dict[actions[x]]
                if obj.get_conf_key_value("installed") == True:
                    actions.remove(codename)
                    actions.insert(x, codename)
                    self.conf.save_conf()
                    print actions
                    i = 0
                    for action in self.conf.get_actions_order():
                        if action == codename:
                            break
                        else:
                            if self.conf.get_action_key_value(action, "installed") == True:
                                i = i+1
                    print ">>> %s" % i
                    return i
            else:
                actions.remove(codename)
                actions.insert(x, codename)
                self.conf.save_conf()
                print actions
                return x
        return 0
        
    def down_action (self, codename):
        actions = self.conf.get_actions_order()
        index = actions.index(codename)
        hide_uninstall = self.conf.get_hide_uninstalled_services()

        list = range(index+1, len(actions))

        for x in list:
            if hide_uninstall == True:
                obj = self.action_objs_dict[actions[x]]
                if obj.get_conf_key_value("installed") == True:
                    actions.remove(codename)
                    actions.insert(x, codename)
                    self.conf.save_conf()
                    print actions
                    i = 0
                    for action in self.conf.get_actions_order():
                        if action == codename:
                            break
                        else:
                            if self.conf.get_action_key_value(action, "installed") == True:
                                i = i+1
                    print ">>> %s" % i
                    return i
            else:
                actions.remove(codename)
                actions.insert(x, codename)
                self.conf.save_conf()
                print actions
                return x
        return len(actions)
        
    def set_connections_model(self, model):
        for x in self.action_objs_dict.keys():
            obj = self.action_objs_dict[x]
            obj.set_connections_model(model)

    def connect_signals(self):
        for x in self.action_objs_dict.keys():
            obj = self.action_objs_dict[x]
            obj.connect_signals()

    def show_actions_conf(self):
        for x in self.action_objs_dict.keys():
            obj = self.action_objs_dict[x]
            obj.show_all()
            
    def launch_action(self, action_codename):
        obj = self.action_objs_dict[action_codename]
        obj.launch()

    def launch_help(self, action_codename):
        obj = self.action_objs_dict[action_codename]
        obj.launch_help()
