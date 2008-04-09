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
import gobject
import MSD
import os
import string

class MSDConnectionsTab:
    def __init__(self, prefs_obj):
        #Connections tab
        self.xml = prefs_obj.xml
        self.conf = prefs_obj.conf
        self.exporter = prefs_obj.exporter
        self.importer = prefs_obj.importer
        
        self.act_manager = prefs_obj.act_manager
        self.connections_dialog = self.xml.get_widget("connections_dialog")
        self.connections_dialog.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
        self.add_connection_button = self.xml.get_widget("add_connection_button")
        self.edit_connection_button = self.xml.get_widget("edit_connection_button")
        self.del_connection_button = self.xml.get_widget("del_connection_button")
        self.export_connection_button = self.xml.get_widget("export_connections")
        self.import_connection_button = self.xml.get_widget("import_connections")
        
        self.connections_main_image = self.xml.get_widget("connections_main_image")
        self.connections_main_image.set_from_file(os.path.join(MSD.icons_files_dir,"connections_32x32.png"))
        self.ask_before_connect_to_action_checkbutton = self.xml.get_widget("ask_before_connect_to_action_check_button")
        self.ask_before_change_connection_checkbutton = self.xml.get_widget("ask_before_change_connection_check_button")
        self.ask_before_connect_checkbutton = self.xml.get_widget("ask_before_connect_check_button")
        self.default_connection_button = self.xml.get_widget("make_default_connection_button")
        image = gtk.Image()
        image.set_from_file(os.path.join(MSD.icons_files_dir,"default_16x16.png"))
        self.default_connection_button.set_image(image)        
        self.connections_treeview = self.xml.get_widget("connections_treeview")
        self.importer.connections_treeview = self.connections_treeview

        self.ask_before_connect_to_action_checkbutton.set_active(self.conf.conf["connections_general"]["ask_before_connect_to_action"])
        self.ask_before_change_connection_checkbutton.set_active(self.conf.conf["connections_general"]["ask_before_change_connection"])
        self.ask_before_connect_checkbutton.set_active(self.conf.conf["connections_general"]["ask_before_connect"])

        #Connection dialog
        self.cd_cancel_button = self.xml.get_widget('connection_cancel_button')
        self.cd_ok_button = self.xml.get_widget('connection_ok_button')
        self.cd_name_entry = self.xml.get_widget('connection_name_entry')
        self.cd_ask_pass_radio_button = self.xml.get_widget('connection_ask_pass_radio_button')
        self.cd_pass_entry = self.xml.get_widget('connection_pass_entry')
        self.cd_user_entry = self.xml.get_widget('connection_user_entry')
        self.cd_custom_profile_radio_button = self.xml.get_widget('connection_custom_profile_radio_button')
        self.cd_default_profile_radio_button = self.xml.get_widget('connection_default_profile_radio_button')
        self.cd_custom_profile_entry = self.xml.get_widget('connection_custom_profile_entry')
        self.cd_auto_dns_radio_button = self.xml.get_widget('connection_auto_dns_radio_button')
        self.cd_not_auto_dns_radio_button = self.xml.get_widget('connection_not_auto_dns_radio_button')
        self.cd_not_auto_dns_info_area = self.xml.get_widget('connection_not_auto_dns_info_area')
        self.cd_primary_dns_entry = self.xml.get_widget('connection_primary_dns_entry')
        self.cd_secondary_dns_entry = self.xml.get_widget('connection_secondary_dns_entry')
        self.cd_dns_domain_entry = self.xml.get_widget('connection_dns_domain_entry')
        self.cd_no_proxy_radio_button = self.xml.get_widget('connection_no_proxy_radio_button')
        self.cd_no_proxy_info_area = self.xml.get_widget('connection_no_proxy_info_area')
        self.cd_proxy_radio_button = self.xml.get_widget('connection_proxy_radio_button')
        self.cd_proxy_ip_entry = self.xml.get_widget('connection_proxy_ip_entry')
        self.cd_proxy_port_entry = self.xml.get_widget('connection_proxy_port_entry')
        self.cd_notebook = self.xml.get_widget('connection_notebook')
        self.cd_is_add_dialog = None
        

        #Populate connections treeview
        liststore = self.connections_treeview.get_model()
        liststore = gtk.ListStore(gtk.gdk.Pixbuf, str, 'gboolean')
        column = gtk.TreeViewColumn('Connections_icon',
                                    gtk.CellRendererPixbuf(),
                                    pixbuf=0)
        self.connections_treeview.append_column(column)
        column = gtk.TreeViewColumn('Connections_name',
                                    gtk.CellRendererText(),
                                    text=1)
        self.connections_treeview.append_column(column)

        default_icon = gtk.gdk.pixbuf_new_from_file (os.path.join(MSD.icons_files_dir,
                                                                  "default_16x16.png"))
        
        default_connection_name = self.conf.get_default_connection_name()
   
        for connection in self.conf.get_connection_names_list():
            if default_connection_name == connection :
                self.default_connection_iter = liststore.prepend([default_icon, connection, True])
            else:
                liststore.prepend([None, connection, False])
        
        self.connections_treeview.set_model(liststore)

        treeselection = self.connections_treeview.get_selection()
        treeselection.set_mode(gtk.SELECTION_SINGLE)
        treeselection.select_path(0)
        

    def connect_signals(self):
        self.connections_dialog.connect("delete_event", self.__delete_event_connections_cb)
        self.add_connection_button.connect("clicked", self.__add_connection_button_cb, None)
        self.edit_connection_button.connect("clicked", self.__edit_connection_button_cb, None)
        self.del_connection_button.connect("clicked", self.__del_connection_button_cb, None)
        self.export_connection_button.connect("clicked", self.__export_connection_button_cb, None)
        self.import_connection_button.connect("clicked", self.__import_connection_button_cb, None)
        self.default_connection_button.connect("clicked", self.__default_connection_button_cb, None)
        self.cd_ok_button.connect("clicked", self.__ok_connection_dialog_cb, None)
        self.cd_cancel_button.connect("clicked", self.__cancel_connection_dialog_cb, None)

        self.ask_before_connect_to_action_checkbutton.connect("toggled", self.__toggle_abc_to_action_cb, None)
        self.ask_before_change_connection_checkbutton.connect("toggled", self.__toggle_abc_conection_cb, None)
        self.ask_before_connect_checkbutton.connect("toggled", self.__toggle_abc_cb, None)

        self.cd_custom_profile_radio_button.connect("toggled", self.__toggle_custom_profile_rb_cb, None)
        self.cd_not_auto_dns_radio_button.connect("toggled", self.__toggle_not_auto_dns_profile_rb_cb, None)
        self.cd_proxy_radio_button.connect("toggled", self.__toggle_proxy_rb_cb, None)
        self.cd_ask_pass_radio_button.connect("toggled", self.__toogle_ask_pass_rb_cb, None)
#       self.handler_id_ppe = self.cd_proxy_port_entry.connect("insert-text", self.__insert_text_proxy_port_entry_cb, None)
        
        treeselection = self.connections_treeview.get_selection()
        treeselection.connect("changed" , self.__connections_treview_row_changed_cb, None)
        self.__connections_treview_row_changed_cb(treeselection, None)

    def show_all(self):
        self.ask_before_connect_to_action_checkbutton.set_active(self.conf.conf["connections_general"]["ask_before_connect_to_action"])
        self.ask_before_change_connection_checkbutton.set_active(self.conf.conf["connections_general"]["ask_before_change_connection"])
        self.ask_before_change_connection_checkbutton.hide()
        self.ask_before_connect_checkbutton.set_active(self.conf.conf["connections_general"]["ask_before_connect"])

##     def __insert_text_proxy_port_entry_cb(self, entry, text, length, *args):  
##         new_text = ''.join([ c for c in text if c in string.digits])
        
##         entry.handler_block(self.handler_id_ppe)
##         pos = entry.get_position()

##         entry.insert_text(new_text, pos)
        
##         entry.handler_unblock(self.handler_id_ppe)
##         gobject.idle_add(lambda: entry.set_position(-1))
##         entry.stop_emission("insert-text")

    def __toogle_ask_pass_rb_cb(self, widget, data):
        if self.cd_ask_pass_radio_button.get_active() == True:
            self.cd_pass_entry.set_sensitive(False)
        else:
            self.cd_pass_entry.set_sensitive(True)
        
    def __toggle_abc_to_action_cb(self, widget, data):
        self.conf.conf["connections_general"]["ask_before_connect_to_action"] = widget.get_active()
        self.conf.save_conf()
        
    def __toggle_abc_conection_cb(self, widget, data):
        self.conf.conf["connections_general"]["ask_before_change_connection"] = widget.get_active()
        self.conf.save_conf()
        
    def __toggle_abc_cb(self, widget, data):
        self.conf.conf["connections_general"]["ask_before_connect"] = widget.get_active()
        self.conf.save_conf()

    def __toggle_custom_profile_rb_cb (self, widget, data):
        if widget.get_active() == True:
            self.cd_custom_profile_entry.set_sensitive(True)
        else:
            self.cd_custom_profile_entry.set_sensitive(False)
    
    def __toggle_not_auto_dns_profile_rb_cb (self, widget, data):
        if widget.get_active() == True:
            self.cd_not_auto_dns_info_area.set_sensitive(True)
        else:
            self.cd_not_auto_dns_info_area.set_sensitive(False)
    
    def __toggle_proxy_rb_cb (self, widget, data):
        if widget.get_active() == True:
            self.cd_no_proxy_info_area.set_sensitive(True)
        else:
            self.cd_no_proxy_info_area.set_sensitive(False)

    def __import_connection_button_cb(self, widget, data):
        dlg = gtk.FileChooserDialog(title=u"Importar conexión",
                                    action=gtk.FILE_CHOOSER_ACTION_OPEN,
                                    buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_OPEN,gtk.RESPONSE_OK))
        dlg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
        dlg.set_current_folder(os.environ["HOME"])

        filter = gtk.FileFilter()
        filter.set_name("Ficheros emsc")
        filter.add_pattern("*.emsc")
        dlg.add_filter(filter)

        filter = gtk.FileFilter()
        filter.set_name("Ficheros msgprs")
        filter.add_pattern("*.msgprs")
        dlg.add_filter(filter)

        ret = dlg.run()
        file = dlg.get_filename()
        dlg.destroy()
        
        if ret == gtk.RESPONSE_OK:
            self.importer.import_from_file(file)
    
    def __export_connection_button_cb(self, widget, data):        
        treeselection = self.connections_treeview.get_selection()
        (model, iter) = treeselection.get_selected()

        p = False
        while p == False:
            dlg = gtk.FileChooserDialog(title=u"Exportar conexión",
                                        action=gtk.FILE_CHOOSER_ACTION_SAVE,
                                        buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_SAVE,gtk.RESPONSE_OK))
            dlg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
            dlg.set_current_folder(os.environ["HOME"])
            dlg.set_current_name(model.get_value(iter, 1).replace("/","_") + ".emsc")
            
            filter = gtk.FileFilter()
            filter.set_name("Ficheros emsc")
            filter.add_pattern("*.emsc")
            dlg.add_filter(filter)

            dlg.set_do_overwrite_confirmation(True)
            
            ret = dlg.run()
            if ret == gtk.RESPONSE_OK:
                if not dlg.get_filename().endswith(".emsc") :
                    msg = gtk.MessageDialog(type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_CLOSE)
                    msg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
                    msg.set_markup(u"<b>El fichero debe tener una extensión</b>")
                    msg.format_secondary_markup(u"Los ficheros que guardes deben tener extensión .emsc")
                    msg.run()
                    msg.destroy()
                    dlg.destroy()
                else:
                    p = True
            else:
                dlg.destroy()
                return

        conn_name =  model.get_value(iter, 1)
        self.exporter.save_connection_to_file(conn_name, dlg.get_filename())
        dlg.destroy()

    def __del_connection_button_cb(self, widget, data):
        treeselection = self.connections_treeview.get_selection()
        (model, iter) = treeselection.get_selected()
        default_conn = model.get_value(iter, 2)
        conn_name =  model.get_value(iter, 1)
        
        dlg = gtk.MessageDialog(type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK_CANCEL)
        dlg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
        dlg.set_markup(MSD.MSG_DELETE_CONNECTION_TITLE)
        dlg.format_secondary_markup(MSD.MSG_DELETE_CONNECTION % gobject.markup_escape_text (conn_name))
        ret = dlg.run()
        dlg.destroy()
        
        if ret != gtk.RESPONSE_OK:
            return
        
        if default_conn == True:
            iter_tmp = model.get_iter(0)
            default_icon = gtk.gdk.pixbuf_new_from_file (os.path.join(MSD.icons_files_dir,
                                                                      "default_16x16.png"))
            while iter_tmp != None:
                if model.get_value(iter_tmp, 1) == "movistar Internet":
                    model.set_value(iter_tmp, 0, default_icon)
                    model.set_value(iter_tmp, 2, True)
                    break
                iter_tmp = model.iter_next(iter_tmp)
        print "Del connection %s " % model.get_value(iter, 1)
        self.conf.del_connection(model.get_value(iter, 1))
        self.conf.save_conf()
        model.remove(iter)
        self.act_manager.show_actions_conf()

    def __default_connection_button_cb(self, widget, data):
        treeselection = self.connections_treeview.get_selection()
        (model, iter) = treeselection.get_selected()
        iter_tmp = model.get_iter(0)        
        while iter_tmp != None:
             model.set_value(iter_tmp, 0, None)
             model.set_value(iter_tmp, 2, False)
             iter_tmp = model.iter_next(iter_tmp)
        default_icon = gtk.gdk.pixbuf_new_from_file (os.path.join(MSD.icons_files_dir,
                                                                  "default_16x16.png"))
        model.set_value(iter, 0, default_icon)
        model.set_value(iter, 2, True)
        self.__connections_treview_row_changed_cb(treeselection, None)
        self.conf.set_default_connection_name(model.get_value(iter, 1))
        self.conf.save_conf()
        
    def __add_connection_button_cb(self, widget, data):
        self.connections_dialog.set_title(MSD.MSG_ADD_CONNECTION_TITLE)
        self.cd_is_add_dialog = True
        print self.conf.conf["connections"]
        
        #Clean dialog
        self.cd_name_entry.set_sensitive(True)
        self.cd_name_entry.set_text("")
        self.cd_pass_entry.set_text("")
        self.cd_user_entry.set_text("")
        self.cd_custom_profile_entry.set_text("")
        
        self.cd_primary_dns_entry.set_text("194.179.1.100")
        self.cd_secondary_dns_entry.set_text("194.179.1.101")
        self.cd_dns_domain_entry.set_text("")
        self.cd_proxy_ip_entry.set_text("")
        self.cd_proxy_port_entry.set_text("0")
        self.cd_ask_pass_radio_button.set_active(False)
        self.cd_default_profile_radio_button.set_active(True)
        self.cd_not_auto_dns_radio_button.set_active(True)
        self.cd_no_proxy_radio_button.set_active(True)

        self.__toggle_custom_profile_rb_cb(self.cd_custom_profile_radio_button, None)
        self.__toggle_not_auto_dns_profile_rb_cb(self.cd_not_auto_dns_radio_button, None)
        self.__toggle_proxy_rb_cb(self.cd_proxy_radio_button, None)

        self.connections_dialog.show_all()
        self.cd_notebook.set_current_page(0)
        self.cd_cancel_button.grab_focus()
        
    def __edit_connection_button_cb(self, widget, data):
        self.connections_dialog.set_title(MSD.MSG_EDIT_CONNECTION_TITLE)
        self.cd_is_add_dialog = False
        self.connections_dialog.show_all()

        #Write all fields in the form
        treeselection = self.connections_treeview.get_selection()
        (model, pathlist) = treeselection.get_selected_rows()
        
        iter =  model.get_iter(pathlist[0])
        self.editing_connection_name = model.get_value(iter, 1)
        
        connection_info = self.conf.get_connection_info_dict(self.editing_connection_name)

        self.cd_name_entry.set_text(self.editing_connection_name)

        if self.editing_connection_name == 'movistar Internet' or self.editing_connection_name == 'movistar Internet directo' :
            self.cd_name_entry.set_sensitive(False)
        else:
            self.cd_name_entry.set_sensitive(True)
            
        #self.cd_name_entry.set_sensitive(False)
        if connection_info["ask_password"] ==  True:
            self.cd_ask_pass_radio_button.set_active(True)
        else:
            self.cd_ask_pass_radio_button.set_active(False)
            
        self.cd_user_entry.set_text(connection_info["user"])
        self.cd_pass_entry.set_text(connection_info["pass"])
            
        if connection_info["profile_by_default"] == True:
            self.cd_default_profile_radio_button.set_active(True)
        else:
            self.cd_custom_profile_radio_button.set_active(True)
            
        if connection_info["profile_name"] != None:
            self.cd_custom_profile_entry.set_text(connection_info["profile_name"])
        else:
            self.cd_custom_profile_entry.set_text("")

        if connection_info["auto_dns"] == True:
            self.cd_auto_dns_radio_button.set_active(True)
        else:
            self.cd_not_auto_dns_radio_button.set_active(True)

        if connection_info["primary_dns"] != None:
            self.cd_primary_dns_entry.set_text(connection_info["primary_dns"])
        else:
            self.cd_primary_dns_entry.set_text("")
        if connection_info["secondary_dns"] != None:
            self.cd_secondary_dns_entry.set_text(connection_info["secondary_dns"])
        else:
            self.cd_secondary_dns_entry.set_text("")
        if connection_info["domains"] != None:    
            self.cd_dns_domain_entry.set_text(connection_info["domains"])
        else:
            self.cd_dns_domain_entry.set_text("")
            
        if connection_info["proxy"] == False:                
            self.cd_no_proxy_radio_button.set_active(True)
        else:
            self.cd_proxy_radio_button.set_active(True)

        if connection_info["proxy_ip"] != None:
            self.cd_proxy_ip_entry.set_text(connection_info["proxy_ip"])
        else:
            self.cd_proxy_ip_entry.set_text("")
        if connection_info["proxy_port"] != None:
            self.cd_proxy_port_entry.set_text(connection_info["proxy_port"])
        else:
            self.cd_proxy_port_entry.set_text("")

        self.__toggle_custom_profile_rb_cb(self.cd_custom_profile_radio_button, None)
        self.__toggle_not_auto_dns_profile_rb_cb(self.cd_not_auto_dns_radio_button, None)
        self.__toggle_proxy_rb_cb(self.cd_proxy_radio_button, None)
            
        self.cd_notebook.set_current_page(0)
        self.cd_ok_button.grab_focus()

    def __delete_event_connections_cb(self, widget, data):
        self.connections_dialog.hide_all()
        return True

    def __connections_treview_row_changed_cb(self, selection, data):
        selection = self.connections_treeview.get_selection()
        (model, iter) = selection.get_selected()
        if iter == None :
            self.edit_connection_button.set_sensitive(False)
            self.del_connection_button.set_sensitive(False)
            self.default_connection_button.set_sensitive(False)
            self.export_connection_button.set_sensitive(False)
        else:
            self.edit_connection_button.set_sensitive(True)
            self.del_connection_button.set_sensitive(True)
            self.export_connection_button.set_sensitive(True)

            if model.get_value(iter, 2) == True :
                self.default_connection_button.set_sensitive(False)
                self.del_connection_button.set_sensitive(False)
            else:
                if model.get_value(iter,1) == "movistar Internet" or model.get_value(iter,1) == "movistar Internet directo":
                    self.default_connection_button.set_sensitive(True)
                    self.del_connection_button.set_sensitive(False)
                else:
                    self.default_connection_button.set_sensitive(True)
                    self.del_connection_button.set_sensitive(True)
                

    def __cancel_connection_dialog_cb(self, widget, data):
        self.connections_dialog.hide_all()

    def __ok_connection_dialog_cb(self, widget, data):
        if self.__add_connection() == True :
            self.connections_dialog.hide_all()

    def __add_connection(self):
        name = self.cd_name_entry.get_text()
        if self.cd_is_add_dialog == True and self.conf.exists_connection(name) == True :
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_WARNING,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
            mesg.set_markup(MSD.MSG_CONNECTION_NAME_EXISTS_TITLE  % name)
            mesg.format_secondary_markup(MSD.MSG_CONNECTION_NAME_EXISTS)
            if (mesg.run() != None):
                mesg.destroy()
                return False
            
        if name == "":
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_WARNING,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
            mesg.set_markup(MSD.MSG_NO_CONNECTION_NAME_TITLE)
            mesg.format_secondary_markup(MSD.MSG_NO_CONNECTION_NAME)
            if (mesg.run() != None):
                mesg.destroy()
                return False

        if (self.cd_is_add_dialog == True and self.conf.exists_connection(name)) or \
           (self.cd_is_add_dialog == False and self.conf.exists_connection(name) and name != self.editing_connection_name):
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_WARNING,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
            mesg.set_markup(MSD.MSG_CONNECTION_NAME_EXISTS_TITLE % name)
            mesg.format_secondary_markup(MSD.MSG_CONNECTION_NAME_EXISTS)
            if (mesg.run() != None):
                mesg.destroy()
                return False
                
        user = self.cd_user_entry.get_text()
        passwd = self.cd_pass_entry.get_text()
        ask_password = self.cd_ask_pass_radio_button.get_active()

        if user == "":
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_WARNING,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
            mesg.set_markup(MSD.MSG_NO_CONNECTION_USER_TITLE)
            mesg.format_secondary_markup(MSD.MSG_NO_CONNECTION_USER)
            if (mesg.run() != None):
                mesg.destroy()
                return False

        if passwd == "" and ask_password == False:
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_WARNING,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
            mesg.set_markup(MSD.MSG_NO_CONNECTION_PASSWORD_TITLE)
            mesg.format_secondary_markup(MSD.MSG_NO_CONNECTION_PASSWORD)
            if (mesg.run() != None):
                mesg.destroy()
                return False

        if ask_password == True:
            passwd = ""

        profile_by_default = self.cd_default_profile_radio_button.get_active()


        profile_name = self.cd_custom_profile_entry.get_text()
        if profile_name == "":
            profile_name = None

        auto_dns = self.cd_auto_dns_radio_button.get_active()
        
        if auto_dns == True:
            if self.cd_primary_dns_entry.get_text() == "":
                primary_dns = None
            else:
                primary_dns = self.cd_primary_dns_entry.get_text()

            if self.cd_secondary_dns_entry.get_text() == "":
                secondary_dns = None
            else:
                secondary_dns = self.cd_secondary_dns_entry.get_text()
        else:
            primary_dns = self.cd_primary_dns_entry.get_text()
            secondary_dns = self.cd_secondary_dns_entry.get_text()
            
            if primary_dns == "" or secondary_dns == "":
                mesg = gtk.MessageDialog(None,
                                         gtk.DIALOG_MODAL,
                                         gtk.MESSAGE_WARNING,
                                         gtk.BUTTONS_CLOSE)
                mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
                mesg.set_markup(MSD.MSG_NO_CONNECTION_DNS_TITLE)
                mesg.format_secondary_markup(MSD.MSG_NO_CONNECTION_DNS)
                if (mesg.run() != None):
                    mesg.destroy()
                    return False

        domains = self.cd_dns_domain_entry.get_text()
        proxy = self.cd_proxy_radio_button.get_active()
                
        if proxy == True:
            proxy_ip = self.cd_proxy_ip_entry.get_text()
            proxy_port = self.cd_proxy_port_entry.get_text()
            
            if proxy_ip == "":
                mesg = gtk.MessageDialog(None,
                                         gtk.DIALOG_MODAL,
                                         gtk.MESSAGE_WARNING,
                                         gtk.BUTTONS_CLOSE)
                mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
                mesg.set_markup(MSD.MSG_NO_CONNECTION_PROXY_IP_TITLE)
                mesg.format_secondary_markup(MSD.MSG_NO_CONNECTION_PROXY_IP)
                if (mesg.run() != None):
                    mesg.destroy()
                    return False
            
            if proxy_port.isdigit() == False:
                mesg = gtk.MessageDialog(None,
                                         gtk.DIALOG_MODAL,
                                         gtk.MESSAGE_WARNING,
                                         gtk.BUTTONS_CLOSE)
                mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
                mesg.set_markup(MSD.MSG_NO_CONNECTION_PROXY_PORT_TITLE)
                mesg.format_secondary_markup(MSD.MSG_NO_CONNECTION_PROXY_PORT)
                if (mesg.run() != None):
                    mesg.destroy()
                    return False
        else:
            if self.cd_proxy_ip_entry.get_text() == "":
                proxy_ip = None
            else:
                proxy_ip = self.cd_proxy_ip_entry.get_text()

            if self.cd_proxy_port_entry.get_text() == "":
                proxy_port = "0"
            else:
                proxy_port = self.cd_proxy_port_entry.get_text()

        if self.cd_is_add_dialog == True:
            default_conn = False
            self.conf.add_connection(name, user, passwd,
                                     ask_password,
                                     profile_by_default,
                                     profile_name,
                                     auto_dns,
                                     primary_dns,
                                     secondary_dns,
                                     domains,
                                     proxy,
                                     proxy_ip,
                                     proxy_port,
                                     default_conn)
        else:
            treeselection = self.connections_treeview.get_selection()
            (model, iter) = treeselection.get_selected()
            default_conn = model.get_value(iter, 2)
            model.set_value(iter,1, name)
            
            self.conf.add_connection(name, user, passwd,
                                     ask_password,
                                     profile_by_default,
                                     profile_name,
                                     auto_dns,
                                     primary_dns,
                                     secondary_dns,
                                     domains,
                                     proxy,
                                     proxy_ip,
                                     proxy_port,
                                     default_conn,
                                     old_name=self.editing_connection_name)
        
        if self.cd_is_add_dialog == True:
            liststore = self.connections_treeview.get_model()
            liststore.append([None, name, False])
        
        self.conf.save_conf()
	return True
