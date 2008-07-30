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
import gobject
import MSD
import os

class MSDBookmarksTab:
    def __init__(self, pref_obj):
        self.xml = pref_obj.xml
        self.conf = pref_obj.conf
        self.exporter = pref_obj.exporter
        self.importer = pref_obj.importer

        #Bookmarks tab
        self.add_bookmark_button = self.xml.get_widget("add_bookmark_button")
        self.edit_bookmark_button = self.xml.get_widget("edit_bookmark_button")
        self.del_bookmark_button = self.xml.get_widget("del_bookmark_button")
        self.export_bookmark_button = self.xml.get_widget("export_bookmark_button")
        self.import_bookmark_button = self.xml.get_widget("import_bookmark_button")
        self.bookmarks_treeview = self.xml.get_widget("bookmarks_treeview")
        self.importer.bookmarks_treeview = self.bookmarks_treeview
        self.bookmarks_main_image = self.xml.get_widget("bookmarks_main_image")
        self.bookmarks_main_image.set_from_file(os.path.join(MSD.icons_files_dir,"bookmarks_32x32.png"))
        
        #Bookmarks dialog
        self.bookmark_dialog = self.xml.get_widget("bookmark_dialog")
        self.bookmark_dialog.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
        self.bd_name_entry = self.xml.get_widget("bookmark_name_entry")
        self.bd_file_radio_button = self.xml.get_widget("bookmark_file_radio_button")
        self.bd_url_radio_button = self.xml.get_widget("bookmark_url_radio_button")
        self.bd_filechooser = self.xml.get_widget("bookmark_filechooser")
        self.bd_url_entry = self.xml.get_widget("bookmark_url_entry")
        self.bd_connection_check = self.xml.get_widget("bookmark_check_connection")
        self.bd_connection_combobox = self.xml.get_widget("bookmark_connection_combobox")
        self.bd_cancel_button = self.xml.get_widget("bookmark_cancel_button")
        self.bd_ok_button = self.xml.get_widget("bookmark_ok_button")
        self.bd_is_add_dialog = None
        self.bd_show_confirmation = False

        #Bookmarks confirmation dialog
        self.bookmark_confirmation_dialog = self.xml.get_widget("bookmark_confirmation_dialog")
        self.bookmark_confirmation_dialog.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
        self.bookmark_confirmation_ok_button = self.xml.get_widget("bookmark_confirmation_ok_button")
        self.bookmark_confirmation_dont_show = self.xml.get_widget("bookmark_confirmation_dont_show")
        self.bookmark_confirmation_icon = self.xml.get_widget("bookmark_confirmation_icon")
        self.bookmark_confirmation_icon.set_from_file(os.path.join(MSD.icons_files_dir, "accesos_directos_48x48.png"))

        #Populate bd_connection_combobox
        liststore = pref_obj.connections_tab.connections_treeview.get_model()
        self.bd_connection_combobox.set_model(liststore)
        cell = gtk.CellRendererText()
        self.bd_connection_combobox.pack_start(cell, True)
        self.bd_connection_combobox.add_attribute(cell, 'text', 1)
        self.bd_connection_combobox.set_active(0)
        
        #Populate bookmarks in the bookmarks tab treeview
        icon_theme = gtk.icon_theme_get_default()
        liststore = self.bookmarks_treeview.get_model()
        liststore = gtk.ListStore(gtk.gdk.Pixbuf, str, 'gdouble')
        column = gtk.TreeViewColumn('Bookmark_icon',
                                    gtk.CellRendererPixbuf(),
                                    pixbuf=0)
        self.bookmarks_treeview.append_column(column)
        column = gtk.TreeViewColumn('Bookmark_name',
                                    gtk.CellRendererText(),
                                    text=1)
        self.bookmarks_treeview.append_column(column)

        liststore.set_sort_column_id(2, gtk.SORT_ASCENDING)

        bookmarks_list = self.conf.get_bookmarks_list()
        
        for bookmark in bookmarks_list:
            pixbuf = self.conf.get_bookmark_icon(bookmark[0])
            timestamp = long (bookmark[4] * 100)
            liststore.append([pixbuf, bookmark[0], timestamp])
        self.bookmarks_treeview.set_model(liststore)
        
        treeselection = self.bookmarks_treeview.get_selection()
        treeselection.set_mode(gtk.SELECTION_SINGLE)
        treeselection.select_path(0)
                                  
        

    def connect_signals(self):
        self.bookmark_dialog.connect("delete_event", self.__delete_event_bookmarks_cb)
        self.add_bookmark_button.connect("clicked", self.__add_bookmark_button_cb, None)
        self.edit_bookmark_button.connect("clicked", self.__edit_bookmark_button_cb, None)
        self.del_bookmark_button.connect("clicked", self.__del_bookmark_button_cb, None)
        self.export_bookmark_button.connect("clicked", self.__export_bookmark_button_cb, None)
        self.import_bookmark_button.connect("clicked", self.__import_bookmark_button_cb, None)

        self.bookmark_confirmation_dialog.connect("delete_event", self.__delete_event_bookmark_confirmation_cb)
        self.bookmark_confirmation_ok_button.connect("clicked", self.__ok_bookmark_confirmation_button_cb, None)
        self.bookmark_confirmation_dont_show.connect("toggled", self.__dont_show_bookmark_confirmation_cb, None)

        self.bd_url_radio_button.connect("toggled", self.__bd_url_radio_button_cb, None)
        self.bd_connection_check.connect("toggled", self.__bd_connection_check_cb, None)
        self.bd_cancel_button.connect("clicked", self.__bd_cancel_button_cb, None)
        self.bd_ok_button.connect("clicked", self.__bd_ok_button_cb, None)

        treeselection = self.bookmarks_treeview.get_selection()
        treeselection.connect("changed" , self.__bookmarks_treview_row_changed_cb, None)
        self.__bookmarks_treview_row_changed_cb(treeselection, None)

    def __bookmarks_treview_row_changed_cb(self, selection, data):
        selection = self.bookmarks_treeview.get_selection()
        (model, iter) = selection.get_selected()

        if iter == None:
            self.add_bookmark_button.set_sensitive(True) 
            self.edit_bookmark_button.set_sensitive(False) 
            self.del_bookmark_button.set_sensitive(False)  
            self.export_bookmark_button.set_sensitive(False)  
            self.import_bookmark_button.set_sensitive(True) 
        else:
            self.add_bookmark_button.set_sensitive(True) 
            self.edit_bookmark_button.set_sensitive(True) 
            self.del_bookmark_button.set_sensitive(True)  
            self.export_bookmark_button.set_sensitive(True)  
            self.import_bookmark_button.set_sensitive(True)


    def __export_bookmark_button_cb(self, widget, data):        
        treeselection = self.bookmarks_treeview.get_selection()
        (model, iter) = treeselection.get_selected()

        p = False
        while p == False:
            dlg = gtk.FileChooserDialog(title=_(u"Exportar acceso directo"),
                                        action=gtk.FILE_CHOOSER_ACTION_SAVE,
                                        buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_SAVE,gtk.RESPONSE_OK))
            dlg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
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
                    msg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
                    msg.set_markup(_(u"<b>El fichero debe tener una extensión</b>"))
                    msg.format_secondary_markup(_(u"Los ficheros que guardes deben tener extensión .emsc"))
                    msg.run()
                    msg.destroy()
                    dlg.destroy()
                else:
                    p = True
            else:
                dlg.destroy()
                return

        bookmark =  model.get_value(iter, 1)
        self.exporter.save_bookmark_to_file(bookmark, dlg.get_filename())
        
        dlg.destroy()

    def __import_bookmark_button_cb(self, widget, data):
        dlg = gtk.FileChooserDialog(title=_(u"Importar acceso directo"),
                                    action=gtk.FILE_CHOOSER_ACTION_OPEN,
                                    buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_OPEN,gtk.RESPONSE_OK))
        dlg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
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

    def __del_bookmark_button_cb(self, widget, data):
        treeselection = self.bookmarks_treeview.get_selection()
        (model, iter) = treeselection.get_selected()
        bookmark_name = model.get_value(iter, 1)

        dlg = gtk.MessageDialog(type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK_CANCEL)
        dlg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
        dlg.set_markup(MSD.MSG_DELETE_BOOKMARK_TITLE)
        dlg.format_secondary_markup(MSD.MSG_DELETE_BOOKMARK % gobject.markup_escape_text (bookmark_name))
        ret = dlg.run()
        dlg.destroy()
        
        if ret != gtk.RESPONSE_OK:
            return

        iter_tmp = model.get_iter(0)
        
        while iter_tmp != None:
            if model.get_value(iter_tmp, 1) == bookmark_name:
                model.remove(iter_tmp)
                break
            iter_tmp = model.iter_next(iter_tmp)
                
        self.conf.del_bookmark(bookmark_name)
        self.conf.save_conf()

    def add_bookmark(self, show_confirmation=False):
        self.bd_show_confirmation = show_confirmation
        self.__add_bookmark_button_cb(self.add_bookmark_button)
        
    def __add_bookmark_button_cb(self, widget, data=None):
        self.bd_is_add_dialog = True
        self.bookmark_dialog.set_title(MSD.MSG_ADD_BOOKMARK_TITLE)

        #Clean the bookmark dialog
        self.bd_name_entry.set_text("")
        self.bd_name_entry.set_sensitive(True)
        self.bd_filechooser.set_filename("")
        self.bd_url_entry.set_text("")
        self.bd_url_radio_button.set_active(True)
        self.bd_connection_check.set_active(False)

        self.__bd_url_radio_button_cb(self.bd_url_radio_button, None)
        self.__bd_connection_check_cb(self.bd_connection_check, None)
        self.bookmark_dialog.show_all()
        self.bd_cancel_button.grab_focus()

    def __edit_bookmark_button_cb(self, widget, data):
        self.bd_is_add_dialog = False
        self.bookmark_dialog.set_title(MSD.MSG_EDIT_BOOKMARK_TITLE)

        #Complete form fields
        treeselection = self.bookmarks_treeview.get_selection()
        (model, iter) = treeselection.get_selected()

        self.editing_bookmark_name = model.get_value(iter, 1)
        self.bd_name_entry.set_text(model.get_value(iter, 1))

        resource = self.conf.get_bookmark_url(model.get_value(iter, 1))
        if resource.startswith("file://") == True:
            self.bd_filechooser.set_uri(resource)
            self.bd_file_radio_button.set_active(True)
            self.bd_url_entry.set_text("")
        else:
            self.bd_filechooser.set_uri("")
            self.bd_url_entry.set_text(resource)
            self.bd_url_radio_button.set_active(True)

        connection_name = self.conf.get_bookmark_connection(model.get_value(iter, 1))
        
        if connection_name == None:
            self.bd_connection_check.set_active(False)
        else:
            self.bd_connection_check.set_active(True)
            liststore = self.bd_connection_combobox.get_model()
            iter_tmp = liststore.get_iter(0)
            
            while iter_tmp != None:
                if connection_name == liststore.get_value(iter_tmp, 1):
                    self.bd_connection_combobox.set_active_iter(iter_tmp)
                iter_tmp = liststore.iter_next(iter_tmp)
        
        self.__bd_url_radio_button_cb(self.bd_url_radio_button, None)
        self.__bd_connection_check_cb(self.bd_connection_check, None)
        self.bookmark_dialog.show_all()
        self.bd_ok_button.grab_focus()

    def __delete_event_bookmarks_cb(self, widget, data):
        self.bookmark_dialog.hide_all()
        return True

    def __bd_url_radio_button_cb (self, widget, data):
        if widget.get_active() == True:
            self.bd_url_entry.set_sensitive(True)
            self.bd_filechooser.set_sensitive(False)
        else:
            self.bd_url_entry.set_sensitive(False)
            self.bd_filechooser.set_sensitive(True)
    
    def __bd_connection_check_cb (self, widget, data):
        if widget.get_active() == True:
            self.bd_connection_combobox.set_sensitive(True)
        else:
            self.bd_connection_combobox.set_sensitive(False)

    def __bd_cancel_button_cb(self, widget, data):
        self.bookmark_dialog.hide_all()

    def __bd_ok_button_cb(self, widget, data):
        if '/' in self.bd_name_entry.get_text():
	    mesg = gtk.MessageDialog(None,
	                             gtk.DIALOG_MODAL,
				     gtk.MESSAGE_WARNING,
				     gtk.BUTTONS_CLOSE)
	    mesg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
	    mesg.set_markup(MSD.MSG_INVALID_BOOKMARK_NAME_TITLE)
	    mesg.format_secondary_markup(MSD.MSG_INVALID_BOOKMARK_NAME)
	    if (mesg.run() != None):
	        mesg.destroy()
        elif self.__add_bookmark() == True:
            self.bookmark_dialog.hide_all()
            # Show confirmation
            if self.bd_is_add_dialog == False or self.bd_show_confirmation == False:
                return
            if (self.conf.get_ui_general_key_value("show_new_bookmark_confirmation") == True):
                self.bookmark_confirmation_dialog.show_all()
                self.bd_show_confirmation = False
    
    def __add_bookmark(self):
        name = self.bd_name_entry.get_text()
        if name == "" :
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_WARNING,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
            mesg.set_markup(MSD.MSG_NO_BOOKMARK_NAME_TITLE)
            mesg.format_secondary_markup(MSD.MSG_NO_BOOKMARK_NAME)
            if (mesg.run() != None):
                mesg.destroy()
                return False
            
        if (self.bd_is_add_dialog == True and self.conf.exists_bookmark(name)) or \
           (self.bd_is_add_dialog == False and self.conf.exists_bookmark(name) and name != self.editing_bookmark_name):
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_WARNING,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
            mesg.set_markup(MSD.MSG_BOOKMARK_NAME_EXISTS_TITLE % name)
            mesg.format_secondary_markup(MSD.MSG_BOOKMARK_NAME_EXISTS )
            if (mesg.run() != None):
                mesg.destroy()
                return False
        
        if self.bd_file_radio_button.get_active():
            resource = self.bd_filechooser.get_uri()
            if resource == None :
                mesg = gtk.MessageDialog(None,
                                         gtk.DIALOG_MODAL,
                                         gtk.MESSAGE_WARNING,
                                         gtk.BUTTONS_CLOSE)
                mesg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
                mesg.set_markup(MSD.MSG_BOOKMARK_NO_FILE_SELECTED_TITLE)
                mesg.format_secondary_markup(MSD.MSG_BOOKMARK_NO_FILE_SELECTED)
                if (mesg.run() != None):
                    mesg.destroy()
                    return False
        else:
            resource = self.bd_url_entry.get_text()
            if resource == "" :
                mesg = gtk.MessageDialog(None,
                                         gtk.DIALOG_MODAL,
                                         gtk.MESSAGE_WARNING,
                                         gtk.BUTTONS_CLOSE)
                mesg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
                mesg.set_markup(MSD.MSG_BOOKMARK_NO_URL_SELECTED_TITLE)
                mesg.format_secondary_markup(MSD.MSG_BOOKMARK_NO_URL_SELECTED)
                if (mesg.run() != None):
                    mesg.destroy()
                    return False          

        if self.bd_connection_check.get_active() == False:
            connection = None
        else:
            model = self.bd_connection_combobox.get_model()
            iter = self.bd_connection_combobox.get_active_iter()
            connection = model.get_value(iter, 1)
            if connection == self.conf.get_default_connection_name():
                connection == None
            

        if self.bd_is_add_dialog == False:
            self.conf.add_bookmark(name, resource, connection, old_name=self.editing_bookmark_name)
        else:
            self.conf.add_bookmark(name, resource, connection)
        self.conf.save_conf()
        
        #Add to the treeview
        if self.bd_is_add_dialog == True :
            liststore = self.bookmarks_treeview.get_model()
            pixbuf = self.conf.get_bookmark_icon(name)
            timestamp = long (self.conf.get_bookmark_timestamp(name)*100)
            liststore.append([pixbuf, name, timestamp])
        else:
            treeselection = self.bookmarks_treeview.get_selection()
            (model, iter) = treeselection.get_selected()
            model.set_value(iter, 1, name)
            pixbuf = self.conf.get_bookmark_icon(name)
            model.set_value(iter, 0, pixbuf)
        return True


    def __delete_event_bookmark_confirmation_cb(self, widget, data):
        self.bookmark_confirmation_dialog.hide_all()
        return True

    def __ok_bookmark_confirmation_button_cb(self, widget, data):
        self.bookmark_confirmation_dialog.hide_all()
        return True

    def __dont_show_bookmark_confirmation_cb(self, widget, data):
        self.conf.set_ui_general_key_value("show_new_bookmark_confirmation", not widget.get_active())
        self.conf.save_conf()
