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
import MSD
import gtk
import gtk.glade
import os
import gnomevfs
import codecs

# extensiones .emsc .msgprs

class MSDImporter:
    def __init__(self, conf):
        self.conf = conf
        self.connection_treeview = None
        self.bookmarks_treeview = None
        self.import_conditions = None
        self.xml = gtk.glade.XML(MSD.glade_files_dir + "import.glade")
        self.imp_dialog = self.xml.get_widget("import_dialog")
        self.imp_dialog.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        self.imp_cancel_button = self.xml.get_widget("import_cancel_button")
        self.imp_ok_button = self.xml.get_widget("import_ok_button")
        self.imp_filename_label = self.xml.get_widget("import_filename_label")
        self.imp_connection_vbox = self.xml.get_widget("import_connection_vbox")
        self.imp_connection_label = self.xml.get_widget("import_connection_label")
        self.imp_can_not_ow_connection_label = self.xml.get_widget("import_can_not_ow_connection_label")
        self.imp_connection_overwrite_alignment = self.xml.get_widget("import_connection_overwrite_alignment")
        self.imp_connection_overwrite_checkbox = self.xml.get_widget("import_connection_overwrite_checkbox")
        self.imp_no_username_entry = self.xml.get_widget("import_no_username_entry")
        self.imp_no_username_vbox = self.xml.get_widget("import_no_username_vbox")
        self.imp_bookmark_vbox = self.xml.get_widget("import_bookmark_vbox")
        self.imp_bookmark_label = self.xml.get_widget("import_bookmark_label")
        self.imp_bookmark_overwrite_alignment = self.xml.get_widget("import_bookmark_overwrite_alignment")
        self.imp_bookmark_overwrite_checkbox = self.xml.get_widget("import_bookmark_overwrite_checkbox")

        #signals !
        self.imp_connection_overwrite_checkbox.connect("toggled", self.__imp_conn_ow_checkbox_cb, None)
        self.imp_bookmark_overwrite_checkbox.connect("toggled", self.__imp_book_ow_checkbox_cb, None)
        self.imp_no_username_entry.connect("changed", self.__imp_no_username_entry_cb, None)

    def __imp_conn_ow_checkbox_cb(self, widget, data):
        print "conn ow"
        self.__check_import_conditions()

    def __imp_book_ow_checkbox_cb(self, widget, data):
        print "book ow"
        self.__check_import_conditions()

    def __imp_no_username_entry_cb(self, editable, data):
        print "editable"
        self.__check_import_conditions()

    def __check_import_conditions(self):
        if self.import_conditions == None:
            return

        if self.import_conditions == "CONN_NOT_EXISTS_AND_USER_EXISTS":
            self.imp_ok_button.set_sensitive(True)

        if self.import_conditions == "CONN_EXISTS_AND_USER_NOT_EXISTS":
            if len(self.imp_no_username_entry.get_text()) > 0 :
                self.imp_ok_button.set_sensitive(True)
            else:
                self.imp_ok_button.set_sensitive(False)

        if self.import_conditions == "OW_CONN_AND_USER_EXISTS":
            if self.imp_connection_overwrite_checkbox.get_active() :
                self.imp_ok_button.set_sensitive(True)
            else:
                self.imp_ok_button.set_sensitive(False)

        if self.import_conditions == "OW_CONN_AND_USER_EXISTS_NOT_EXISTS":
            if self.imp_connection_overwrite_checkbox.get_active():
                self.imp_no_username_entry.set_sensitive(True)
            else:
                self.imp_no_username_entry.set_sensitive(False)
            
            if len(self.imp_no_username_entry.get_text()) > 0 and self.imp_connection_overwrite_checkbox.get_active():
                self.imp_ok_button.set_sensitive(True)
            else:
                self.imp_ok_button.set_sensitive(False)

        if self.import_conditions == "BOOKMARK_NOT_EXISTS":
            self.imp_ok_button.set_sensitive(True)

        if self.import_conditions == "OW_BOOKMARK":
            if self.imp_bookmark_overwrite_checkbox.get_active():
                self.imp_ok_button.set_sensitive(True)
            else:
                self.imp_ok_button.set_sensitive(False)

        if self.import_conditions == "CONN_NOT_EXISTS_AND_BOOKMARK_NOT_EXISTS_USER_EXISTS":
            self.imp_ok_button.set_sensitive(True)

        if self.import_conditions == "CONN_NOT_EXISTS_AND_BOOKMARK_NOT_EXISTS_USER_NOT_EXISTS":
            if len(self.imp_no_username_entry.get_text()) > 0:
                self.imp_ok_button.set_sensitive(True)
            else:
                self.imp_ok_button.set_sensitive(False) 

        if self.import_conditions == "CONN_INTERNETGPRS3G_AND_OW_BOOKMARK":
            if self.imp_bookmark_overwrite_checkbox.get_active():
                self.imp_ok_button.set_sensitive(True)
            else:
                self.imp_ok_button.set_sensitive(False)

        if self.import_conditions == "OW_CONN_AND_OW_BOOKMARK_AND_USER_EXISTS":
            if self.imp_connection_overwrite_checkbox.get_active() or self.imp_bookmark_overwrite_checkbox.get_active():
                self.imp_ok_button.set_sensitive(True)
            else:
                self.imp_ok_button.set_sensitive(False)

        if self.import_conditions == "OW_CONN_AND_OW_BOOKMARK_AND_USER_NOT_EXISTS":
            if self.imp_connection_overwrite_checkbox.get_active() and self.imp_bookmark_overwrite_checkbox.get_active():
                self.imp_no_username_entry.set_sensitive(True)
                if len(self.imp_no_username_entry.get_text())>0 :
                    self.imp_ok_button.set_sensitive(True)
                else:
                    self.imp_ok_button.set_sensitive(False)
            else:
                if not self.imp_connection_overwrite_checkbox.get_active() and not self.imp_bookmark_overwrite_checkbox.get_active():
                    self.imp_ok_button.set_sensitive(False)
                    self.imp_no_username_entry.set_sensitive(False)
                else:
                    if self.imp_connection_overwrite_checkbox.get_active():
                        self.imp_no_username_entry.set_sensitive(True)
                        if len(self.imp_no_username_entry.get_text())>0 :
                            self.imp_ok_button.set_sensitive(True)
                        else:
                            self.imp_ok_button.set_sensitive(False)
                    else:
                        self.imp_ok_button.set_sensitive(True)
                        self.imp_no_username_entry.set_sensitive(False)

        if self.import_conditions == "CONN_NOT_EXIST_AND_OW_BOOKMARK_AND_USER_EXISTS":
            self.imp_ok_button.set_sensitive(True)

        if self.import_conditions == "CONN_NOT_EXIST_AND_OW_BOOKMARK_AND_USER_NOT_EXISTS":
            if len(self.imp_no_username_entry.get_text())>0 :
                self.imp_ok_button.set_sensitive(True)
            else:
                self.imp_ok_button.set_sensitive(False)

        if self.import_conditions == "CONN_INTERNETGRPS3G_AND_BOOKMARK_NOT_EXISTS":
            self.imp_ok_button.set_sensitive(True)

        if self.import_conditions == "OW_CONN_AND_BOOKMARK_NOT_EXISTS_AND_USER_EXISTS":
            self.imp_ok_button.set_sensitive(True)

        if self.import_conditions == "OW_CONN_AND_BOOKMARK_NOT_EXISTS_AND_USER_NOT_EXISTS":
            if self.imp_connection_overwrite_checkbox.get_active():
                self.imp_no_username_entry.set_sensitive(True)
            else:
                self.imp_no_username_entry.set_sensitive(False)
            if self.imp_connection_overwrite_checkbox.get_active() and len(self.imp_no_username_entry.get_text()) == 0:
                self.imp_ok_button.set_sensitive(False)
            else:
                self.imp_ok_button.set_sensitive(True)
        
        
    def import_from_file(self, file):
        self.file = os.path.basename(file)

        fd = codecs.open(file, "r", "latin-1")

        dict = {}
        for line in fd.readlines():
            par = line.split(" =")
            dict[par[0]] = par[1].lstrip(" ").strip("\r\n").strip("\n").encode("utf-8")
        print "---------------------"
        print dict
        print repr(dict)
        print "---------------------"
        
        fd.close()

        has_bookmark_info = self.__has_bookmark_info(dict)
        has_connection_info = self.__has_connection_info(dict)

        if has_bookmark_info and has_connection_info :
            bookmark, conn = self.__import_bookmark_and_connection(dict)
            if bookmark == None or conn == None :
                return False
            self.__add_bookmark_and_connection(dict, bookmark, conn)
        else:
            if has_bookmark_info == True:
                bookmark = self.__import_bookmark(dict)
                if bookmark == None:
                    return False
                self.__add_bookmark(dict, bookmark)
            else:
                conn = self.__import_connection(dict)
                if conn == None:
                    return False
                self.__add_connection(dict, conn)

    def __add_connection_to_conf(self, conn):
        self.conf.add_connection(conn["name"], conn["user"], conn["pass"],
                                 ask_password = conn["ask_password"],
                                 profile_by_default = conn["profile_by_default"],
                                 profile_name = conn["profile_name"],
                                 auto_dns = conn["auto_dns"],
                                 primary_dns = conn["primary_dns"],
                                 secondary_dns = conn["secondary_dns"],
                                 domains = conn["domains"],
                                 proxy = conn["proxy"],
                                 proxy_ip = conn["proxy_ip"],
                                 proxy_port = conn["proxy_port"],
                                 default = False)
        self.conf.save_conf()
        liststore = self.connections_treeview.get_model()
        liststore.append([None, conn["name"], False])

    def __ow_connection_to_conf(self, conn):
        if conn["name"] == self.conf.get_default_connection_name():
            by_default = True
        else:
            by_default = False

            
        self.conf.add_connection(conn["name"], conn["user"], conn["pass"],
                                 ask_password = conn["ask_password"],
                                 profile_by_default = conn["profile_by_default"],
                                 profile_name = conn["profile_name"],
                                 auto_dns = conn["auto_dns"],
                                 primary_dns = conn["primary_dns"],
                                 secondary_dns = conn["secondary_dns"],
                                 domains = conn["domains"],
                                 proxy = conn["proxy"],
                                 proxy_ip = conn["proxy_ip"],
                                 proxy_port = conn["proxy_port"],
                                 default = by_default)
        self.conf.save_conf()
        

    def __add_bookmark_to_conf(self, bookmark, conn=None):
        print "__add_bookmark_to_conf"
        print bookmark
        print "----------------------------------------"
        if bookmark["uri"] == None:
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_ERROR,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
            mesg.set_markup(u"<b>Este acceso directo no puede ser importado</b>")
            mesg.format_secondary_markup("Error al importar el acceso directo. Probablemente el archivo asociado no exista en tu sistema.")
            
            if (mesg.run() != None):
                mesg.destroy()
                return

        bookmark_name = bookmark["name"]
        self.conf.add_bookmark(bookmark_name, bookmark["uri"], conn)
        self.conf.save_conf()
        
        liststore = self.bookmarks_treeview.get_model()
        pixbuf = self.conf.get_bookmark_icon(bookmark_name)
        timestamp = long (self.conf.get_bookmark_timestamp(bookmark_name)*100)
        liststore.append([pixbuf, bookmark_name, timestamp])

    def __ow_bookmark_to_conf(self, bookmark, conn=None):
        print "__add_bookmark_to_conf"
        print bookmark
        print "----------------------------------------"
        if bookmark["uri"] == None:
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_ERROR,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
            mesg.set_markup(u"<b>Este acceso directo no puede ser importado</b>")
            mesg.format_secondary_markup("Error al importar el acceso directo. Probablemente el archivo asociado no exista en tu sistema.")
            
            if (mesg.run() != None):
                mesg.destroy()
                return

        bookmark_name = bookmark["name"]
        
        self.conf.add_bookmark(bookmark_name, bookmark["uri"], conn)
        self.conf.save_conf()
            
        liststore = self.bookmarks_treeview.get_model()
        tmp_iter = liststore.get_iter(0)
            
        while tmp_iter != None:
            tmp_name = liststore.get_value(tmp_iter, 1)
            if tmp_name == bookmark_name:
                timestamp = long (self.conf.get_bookmark_timestamp(bookmark_name)*100)
                liststore.set_value(tmp_iter, 2, timestamp)
                liststore.set_value(tmp_iter, 0, self.conf.get_bookmark_icon(bookmark_name))
            tmp_iter = liststore.iter_next(tmp_iter)

    def __show_parts_of_import_dialog(self,
                                      conn_label=False,
                                      conn_cant_ow=False,
                                      conn_ow=False,
                                      conn_no_username=False,
                                      book_label=False,
                                      book_ow=False
                                      ):
        
        self.imp_connection_vbox.hide_all()
        self.imp_bookmark_vbox.hide_all()

        if conn_label == True or conn_cant_ow == True or conn_ow == True or conn_no_username ==True:
            self.imp_connection_vbox.show()

        if book_label == True or book_ow == True:
            self.imp_bookmark_vbox.show()

        if conn_label == True:
            self.imp_connection_label.show()

        if conn_cant_ow == True:
            self.imp_can_not_ow_connection_label.show()

        if conn_ow == True:
            self.imp_connection_overwrite_alignment.show_all()
            self.imp_connection_overwrite_checkbox.set_active(False)
            self.imp_connection_overwrite_checkbox.show()

        if conn_no_username == True:
            self.imp_no_username_vbox.show_all()
            self.imp_no_username_entry.set_text("")

        if book_label == True:
            self.imp_bookmark_label.show()
            
        if book_ow == True:
            self.imp_bookmark_overwrite_alignment.show_all()
            self.imp_bookmark_overwrite_checkbox.set_active(False)
            self.imp_bookmark_overwrite_checkbox.show()
        
        

    def __add_connection(self, dict, conn):
        print conn
        # BECAREFUL WITH USER FIELD !!!!!
        # if not exist (conn):
        #    add conn
        # else:
        #    if conn == "Internet GPRS":
        #       dialog ("No puedes añadir esta conexion")
        #    else:
        #       ask (puedo sobreescribir esta conn)

        if not self.conf.exists_connection(conn["name"]) :
            if conn["user"] != None:
                self.import_conditions = "CONN_NOT_EXISTS_AND_USER_EXISTS"
                self.imp_filename_label.set_markup(u"%s" % self.file)
                print "---------------------------------------"
                print conn["name"]
                print repr(conn["name"])
                print "---------------------------------------"
                self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                self.__show_parts_of_import_dialog(conn_label=True)
                self.imp_dialog.show()

                self.__check_import_conditions()
                ret = self.imp_dialog.run()
                self.imp_dialog.hide()
                if ret != gtk.RESPONSE_OK:
                    print "Importacion cancelada"
                    return
                
                self.__add_connection_to_conf(conn)
            else:
                print "Caso de que no haya usuario"
                self.import_conditions = "CONN_EXISTS_AND_USER_NOT_EXISTS"
                self.imp_filename_label.set_markup(u"%s" % self.file)
                print "---------------------------------------"
                print conn["name"]
                print repr(conn["name"])
                print "---------------------------------------"
                self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                self.__show_parts_of_import_dialog(conn_label=True, conn_no_username=True)
                self.imp_dialog.show()

                self.__check_import_conditions()
                ret = self.imp_dialog.run()
                self.imp_dialog.hide()
                if ret != gtk.RESPONSE_OK:
                    print "Importacion cancelada"
                    return

                conn["user"] = self.imp_no_username_entry.get_text()
                self.__add_connection_to_conf(conn)
                
        else:
            if conn["name"] == "movistar Internet" or conn["name"] == "movistar Internet directo" :
                mesg = gtk.MessageDialog(None,
                                         gtk.DIALOG_MODAL,
                                         gtk.MESSAGE_ERROR,
                                         gtk.BUTTONS_CLOSE)
                mesg.set_icon_from_file(MSD.icons_files_dir + "connections_16x16.png")
                mesg.set_markup(u"<b>La conexión '%s' no se puede sobrescribir</b>" % conn["name"])
                if (mesg.run() != None):
                    mesg.destroy()
                    return
            else:
                print "preguntar la conexion, si desea ser sobreescrita"
                if conn["user"] != None:
                    self.import_conditions = "OW_CONN_AND_USER_EXISTS"
                    self.imp_filename_label.set_markup(u"%s" % self.file)
                    self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                    self.__show_parts_of_import_dialog(conn_label=True, conn_ow=True)
                    self.imp_dialog.show()

                    self.__check_import_conditions()
                    ret = self.imp_dialog.run()
                    self.imp_dialog.hide()
                    if ret != gtk.RESPONSE_OK:
                        print "Importacion cancelada"
                        return

                    self.__ow_connection_to_conf(conn)
                    
                else:
                    self.import_conditions = "OW_CONN_AND_USER_EXISTS_NOT_EXISTS"
                    self.imp_filename_label.set_markup(u"%s" % self.file)
                    self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                    self.__show_parts_of_import_dialog(conn_label=True, conn_no_username=True, conn_ow=True)
                    self.imp_dialog.show()

                    self.__check_import_conditions()
                    ret = self.imp_dialog.run()
                    self.imp_dialog.hide()
                    if ret != gtk.RESPONSE_OK:
                        print "Importacion cancelada"
                        return

                    conn["user"] = self.imp_no_username_entry.get_text()
                    self.__ow_connection_to_conf(conn)

    def __add_bookmark(self, dict, bookmark):
        print bookmark
        # if not exists(bookmark):
        #    add
        # else:
        #    ask (puedo sobreescribir este bookmark)
        #

        if bookmark["uri"] == None:
            mesg = gtk.MessageDialog(None,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_ERROR,
                                     gtk.BUTTONS_CLOSE)
            mesg.set_icon_from_file(MSD.icons_files_dir + "bookmarks_16x16.png")
            mesg.set_markup(u"<b>Este acceso directo no puede ser importado</b>")
            mesg.format_secondary_markup("Error al importar el acceso directo. Probablemente el archivo asociado no exista en tu sistema.")
            
            if (mesg.run() != None):
                mesg.destroy()
                return

        if not self.conf.exists_bookmark(bookmark["name"]) :
            self.import_conditions = "BOOKMARK_NOT_EXISTS"
            self.imp_filename_label.set_markup(u"%s" % self.file)
            self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
            self.__show_parts_of_import_dialog(book_label=True)
            self.imp_dialog.show()

            self.__check_import_conditions()
            ret = self.imp_dialog.run()
            self.imp_dialog.hide()
            if ret != gtk.RESPONSE_OK:
                print "Importacion cancelada"
                return

            self.__add_bookmark_to_conf(bookmark)
        else:
            print "preguntar si se puede sobreescribir el acceso directo"
            self.import_conditions = "OW_BOOKMARK"
            self.imp_filename_label.set_markup(u"%s" % self.file)
            self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
            self.__show_parts_of_import_dialog(book_label=True, book_ow=True)
            self.imp_dialog.show()

            self.__check_import_conditions()
            ret = self.imp_dialog.run()
            self.imp_dialog.hide()
            if ret != gtk.RESPONSE_OK:
                print "Importacion cancelada"
                return
            
            self.__ow_bookmark_to_conf(bookmark)

            

    def __add_bookmark_and_connection(self, dict, bookmark, conn):
        print bookmark
        print conn
        # BECAREFUL WITH USER FIELD !!!!!
        # if not ( exist(bookmark) and exist(conn) ):
        #    add (bookmark)
        #    add (conn)
        # else:
        #    if existe(bookmark) and exist(conn):
        #       ask(Puedo sobreescribir bookmark y conn)
        #    else:
        #       if exist(bookmark):
        #          ask (Puedo sobreescribir bookmark)
        #       else:
        #          ask (Puedo sobreescribir conn)

        if not self.conf.exists_bookmark(bookmark["name"]) and not self.conf.exists_connection(conn["name"]):
            print "add bookmark & conn"
            if conn["user"] != None:
                self.import_conditions = "CONN_NOT_EXISTS_AND_BOOKMARK_NOT_EXISTS_USER_EXISTS"
                print self.import_conditions
                self.imp_filename_label.set_markup(u"%s" % self.file)
                self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                self.__show_parts_of_import_dialog(conn_label=True, book_label=True)
                self.imp_dialog.show()

                self.__check_import_conditions()
                ret = self.imp_dialog.run()
                self.imp_dialog.hide()
                if ret != gtk.RESPONSE_OK:
                    print "Importacion cancelada"
                    return

                self.__add_connection_to_conf(conn)
                self.__add_bookmark_to_conf(bookmark, conn=conn["name"])
            else:
                self.import_conditions = "CONN_NOT_EXISTS_AND_BOOKMARK_NOT_EXISTS_USER_NOT_EXISTS"
                print self.import_conditions
                self.imp_filename_label.set_markup(u"%s" % self.file)
                self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                self.__show_parts_of_import_dialog(conn_label=True, book_label=True, conn_no_username=True)
                self.imp_dialog.show()

                self.__check_import_conditions()
                ret = self.imp_dialog.run()
                self.imp_dialog.hide()
                if ret != gtk.RESPONSE_OK:
                    print "Importacion cancelada"
                    return
                conn["user"] = self.imp_no_username_entry.get_text()
                self.__add_connection_to_conf(conn)
                self.__add_bookmark_to_conf(bookmark, conn=conn["name"])
                
        else:
            if self.conf.exists_bookmark(bookmark["name"]) and self.conf.exists_connection(conn["name"]):
                if conn["name"] == "movistar Internet" or conn["name"] == "movistar Internet directo" :
                    print "ask(Puedo sobreescribir bookmark y conn no porque es internt/gprs)"
                    self.import_conditions = "CONN_INTERNETGPRS3G_AND_OW_BOOKMARK"
                    print self.import_conditions
                    self.imp_filename_label.set_markup(u"%s" % self.file)
                    self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                    self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                    self.__show_parts_of_import_dialog(conn_label=True, conn_cant_ow=True, book_label=True, book_ow=True)
                    self.imp_dialog.show()
                    
                    self.__check_import_conditions()
                    ret = self.imp_dialog.run()
                    self.imp_dialog.hide()
                    if ret != gtk.RESPONSE_OK:
                        print "Importacion cancelada"
                        return

                    self.__ow_bookmark_to_conf(bookmark, conn=conn["name"])
                
                else:
                    if conn["user"] != None:
                        print "ask(Puedo sobreescribir bookmark y conn)"
                        self.import_conditions = "OW_CONN_AND_OW_BOOKMARK_AND_USER_EXISTS"
                        print self.import_conditions
                        self.imp_filename_label.set_markup(u"%s" % self.file)
                        self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                        self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                        self.__show_parts_of_import_dialog(conn_label=True, conn_ow=True, book_label=True, book_ow=True)
                        self.imp_dialog.show()
                        
                        self.__check_import_conditions()
                        ret = self.imp_dialog.run()
                        self.imp_dialog.hide()
                        if ret != gtk.RESPONSE_OK:
                            print "Importacion cancelada"
                            return

                        if self.imp_connection_overwrite_checkbox.get_active():
                            self.__ow_connection_to_conf(conn)

                        book_conn = conn["name"]
                        
                        if self.imp_bookmark_overwrite_checkbox.get_active():
                            self.__ow_bookmark_to_conf(bookmark, conn=book_conn)
                        
                    else:
                        print "ask(Puedo sobreescribir bookmark y conn, sin user)"
                        self.import_conditions = "OW_CONN_AND_OW_BOOKMARK_AND_USER_NOT_EXISTS"
                        print self.import_conditions
                        self.imp_filename_label.set_markup(u"%s" % self.file)
                        self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                        self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                        self.__show_parts_of_import_dialog(conn_label=True, conn_ow=True, conn_no_username=True, book_label=True, book_ow=True)
                        self.imp_dialog.show()
                        
                        self.__check_import_conditions()
                        ret = self.imp_dialog.run()
                        self.imp_dialog.hide()
                        if ret != gtk.RESPONSE_OK:
                            print "Importacion cancelada"
                            return

                        if self.imp_connection_overwrite_checkbox.get_active():
                            conn["user"] = self.imp_no_username_entry.get_text()
                            self.__ow_connection_to_conf(conn)

                        book_conn = conn["name"]

                        if self.imp_bookmark_overwrite_checkbox.get_active():
                            self.__ow_bookmark_to_conf(bookmark, conn=book_conn)
                        
            else:
                if self.conf.exists_bookmark(bookmark["name"]):
                    print "ask (Puedo sobreescribir bookmark)"
                    if conn["user"] != None:
                        self.import_conditions = "CONN_NOT_EXIST_AND_OW_BOOKMARK_AND_USER_EXISTS"
                        print self.import_conditions
                        self.imp_filename_label.set_markup(u"%s" % self.file)
                        self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                        self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                        self.__show_parts_of_import_dialog(conn_label=True, book_label=True, book_ow=True)
                        self.imp_dialog.show()
                    
                        self.__check_import_conditions()
                        ret = self.imp_dialog.run()
                        self.imp_dialog.hide()
                        if ret != gtk.RESPONSE_OK:
                            print "Importacion cancelada"
                            return

                        self.__add_connection_to_conf(conn)
                        
                        if self.imp_bookmark_overwrite_checkbox.get_active():
                            self.__ow_bookmark_to_conf(bookmark, conn=conn["name"])
                            
                    else:
                        self.import_conditions = "CONN_NOT_EXIST_AND_OW_BOOKMARK_AND_USER_NOT_EXISTS"
                        print self.import_conditions
                        self.imp_filename_label.set_markup(u"%s" % self.file)
                        self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                        self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                        self.__show_parts_of_import_dialog(conn_label=True, conn_no_username=True, book_label=True, book_ow=True)
                        self.imp_dialog.show()
                        
                        self.__check_import_conditions()
                        ret = self.imp_dialog.run()
                        self.imp_dialog.hide()
                        if ret != gtk.RESPONSE_OK:
                            print "Importacion cancelada"
                            return
                            
                        conn["user"] = self.imp_no_username_entry.get_text()
                        self.__add_connection_to_conf(conn)

                        if self.imp_bookmark_overwrite_checkbox.get_active():
                            self.__ow_bookmark_to_conf(bookmark, conn=conn["name"])
                        
                else:
                    if conn["name"] == "movistar Internet" or conn["name"] == "movistar Internet directo" :
                        print "ask (Puedo no sobreescribir porque es internet gprs3g )"
                        self.import_conditions = "CONN_INTERNETGRPS3G_AND_BOOKMARK_NOT_EXISTS"
                        print self.import_conditions
                        self.imp_filename_label.set_markup(u"%s" % self.file)
                        self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                        self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                        self.__show_parts_of_import_dialog(conn_label=True, conn_cant_ow=True, book_label=True)
                        self.imp_dialog.show()
                        
                        self.__check_import_conditions()
                        ret = self.imp_dialog.run()
                        self.imp_dialog.hide()
                        if ret != gtk.RESPONSE_OK:
                            print "Importacion cancelada"
                            return

                        self.__add_bookmark_to_conf(bookmark, conn=conn["name"])
                        
                    else:
                        print "ask (Puedo sobreescribir conn)"
                        if conn["user"] != None:
                            self.import_conditions = "OW_CONN_AND_BOOKMARK_NOT_EXISTS_AND_USER_EXISTS"
                            print self.import_conditions
                            self.imp_filename_label.set_markup(u"%s" % self.file)
                            self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                            self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                            self.__show_parts_of_import_dialog(conn_label=True, conn_ow=True, book_label=True)
                            self.imp_dialog.show()
                        
                            self.__check_import_conditions()
                            ret = self.imp_dialog.run()
                            self.imp_dialog.hide()
                            if ret != gtk.RESPONSE_OK:
                                print "Importacion cancelada"
                                return

                            if self.imp_connection_overwrite_checkbox.get_active():
                                self.__ow_connection_to_conf(conn)

                            book_conn = conn["name"]
                            self.__add_bookmark_to_conf(bookmark, conn=book_conn)

                        else:
                            self.import_conditions = "OW_CONN_AND_BOOKMARK_NOT_EXISTS_AND_USER_NOT_EXISTS"
                            print self.import_conditions
                            self.imp_filename_label.set_markup(u"%s" % self.file)
                            self.imp_connection_label.set_markup(u"<b>Conexión : '%s'</b>" % conn["name"])
                            self.imp_bookmark_label.set_markup(u"<b>Acceso directo : '%s'</b>" % bookmark["name"])
                            self.__show_parts_of_import_dialog(conn_label=True, conn_ow=True, conn_no_username=True, book_label=True)
                            self.imp_dialog.show()
                        
                            self.__check_import_conditions()
                            ret = self.imp_dialog.run()
                            self.imp_dialog.hide()
                            if ret != gtk.RESPONSE_OK:
                                print "Importacion cancelada"
                                return

                            if self.imp_connection_overwrite_checkbox.get_active():
                                conn["user"] = self.imp_no_username_entry.get_text()
                                self.__ow_connection_to_conf(conn)
                            book_conn = conn["name"]

                            self.__add_bookmark_to_conf(bookmark, conn=book_conn)
                                                        

    def __import_bookmark_and_connection(self, dict):
        bookmark = self.__import_bookmark(dict)
        conn = self.__import_connection(dict)
        return (bookmark, conn)

    def __import_bookmark(self, dict):
        needed_fields = [u'Abrir fichero', u'Abrir url', u'Fichero inicio', u'Url inicio']
        print "---------------------------------"
        print dict
        print repr(dict)
        print "----------------------------------"
        
        for field in needed_fields:
            if dict.has_key(field) == False:
                self.__error_importing("Falta el campo %s en el fichero" % field)
                return None
    
        bookmark = {"name" : dict["Nombre servicio"]}
        if dict[u"Abrir fichero"] == "1" :
            bookmark["file"] = True
            try:
                if gnomevfs.exists(dict["Fichero inicio"]) == 1:
                    tmp_file =  dict["Fichero inicio"]
                    if tmp_file.startswith("file://"):
                        bookmark["uri"] = tmp_file
                    else:
                        bookmark["uri"] = "file://" + dict["Fichero inicio"]
                else:
                    bookmark["uri"] = None
            except:
                bookmark["uri"] = None
        else:
            bookmark["file"] = False
            bookmark["uri"] = dict["Url inicio"]
        
        return bookmark

    def __import_connection(self, dict):
        needed_fields = [u'APN',
                         u'Clave al conectar',
                         u'Clave usuario',
                         u'Configuración proxy',
                         u'DNS primario',
                         u'DNS secundario',
                         u'Nombre usuario',
                         u'Proxy',
                         u'Puerto proxy',
                         u'Servicio',
                         u'Sufijos DNS',
                         u'Usa APN',
                         u'Usa DNS']

        print "---------------------------------"
        print dict
        print repr(dict)
        print "----------------------------------"
        
        for field in needed_fields:
            if dict.has_key(field) == False:
                self.__error_importing("Falta el campo %s en el fichero" % field)
                return None

        conn = {"name" : dict["Servicio"]}
            
        if dict["Nombre usuario"] != "" :
            conn["user"] = dict["Nombre usuario"]
        else:
            conn["user"] = None

        if dict[u"Clave al conectar"] == "1":
            conn["ask_password"] = True
            conn["pass"] = ""
        else:
            conn["ask_password"] = False
            conn["pass"] = MSD.MSDUtils.decode_password(dict["Clave usuario"])
            if conn["pass"] == "":
                conn["ask_password"] = True

        if dict[u"Usa APN"] == "1":
            conn["profile_by_default"] = False
            conn["profile_name"] = dict["APN"]
        else:
            conn["profile_by_default"] = True
            conn["profile_name"] = None

        if dict[u"Usa DNS"] == "1":
            conn["auto_dns"] = False
            conn["primary_dns"] = dict["DNS primario"]
            conn["secondary_dns"] = dict["DNS secundario"]
        else:
            conn["auto_dns"] = True
            conn["primary_dns"] = ""
            conn["secondary_dns"] = ""
            
        if len(dict[u"Sufijos DNS"]) > 0 :
            conn["domains"] = dict["Sufijos DNS"]
        else:
            conn["domains"] = None

        if dict[u"Configuración proxy"] == "1":
            conn["proxy"] = True
            conn["proxy_ip"] = dict["Proxy"]
            conn["proxy_port"] = dict["Puerto proxy"]
        else:
            conn["proxy"] = False
            conn["proxy_ip"] = None
            conn["proxy_port"] = "0"

        return conn
    
    def __has_bookmark_info(self, dict):
        if dict.has_key("Nombre servicio"):
            return True
        else:
            return False

    def __has_connection_info(self, dict):
        if dict.has_key("Servicio"):
            return True
        else:
            return False

    def __error_importing(self, msg):
        print "Error importando : %s" % msg
