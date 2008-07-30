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
import dbus
import dbus.glib
from MSD.MSDPPPvariables import *
import gobject
import os
import MobileManager

from MobileManager.MobileManagerDbus import MOBILE_MANAGER_CONTROLLER_PATH,MOBILE_MANAGER_CONTROLLER_URI,MOBILE_MANAGER_CONTROLLER_INTERFACE_URI,MOBILE_MANAGER_DEVICE_PATH,MOBILE_MANAGER_DEVICE_URI,MOBILE_MANAGER_DEVICE_INFO_INTERFACE_URI,MOBILE_MANAGER_DEVICE_AUTH_INTERFACE_URI,MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI,MOBILE_MANAGER_DEVICE_XZONE_INTERFACE_URI

STATUS_MESSAGES = [_(u"Desconectado"),_(u"Conectado"),_(u"Conectando"),_(u"Desconectando")]
BUTTON_TITLES = [_(u"Conectar"),_(u"Desconectar"),_(u"Conectar"),_(u"Desconectar")]

class MSDConnectionsManager:
    def __init__(self, conf, main_window):
        
        self.conf = conf
        self.main_window = main_window
        self.main_stats = self.main_window.main_stats
        self.main_connect_button = self.main_window.connect_button
        self.main_disconnect_button = self.main_window.disconnect_button
        self.status_bar = self.main_window.main_statusbar
        self.connect_statusbar_im = self.main_window.connect_statusbar_im
        self.disconnect_statusbar_im = self.main_window.disconnect_statusbar_im
        self.mcontroller = self.main_window.mcontroller
        self.ppp_manager = None 
        self.actual_connection = None
        self.waiting_connection = None
        self.starting_connection = None
        self.connection_successful = None
        self.action = None
        self.bookmark_info = None
        self.cardmanager = None
        self.reconnecting_flag = False
        self.abort_now_flag = False
        self.stats_flag = False
        self.connect_with_phone = None

        #Ask pass dialog
        self.ask_xml = gtk.glade.XML(MSD.glade_files_dir + "ask_password.glade")
        self.ask_pass_dialog = self.ask_xml.get_widget("ask_pass_dialog")
        self.ask_pass_label = self.ask_xml.get_widget("ask_pass_label")
        self.ask_pass_entry = self.ask_xml.get_widget("ask_pass_entry")
        self.ask_pass_ok_button = self.ask_xml.get_widget("ask_pass_ok_button")
        self.ask_pass_cancel_button = self.ask_xml.get_widget("ask_pass_cancel_button")
        self.ask_pass_entry.connect("changed", self.__ask_pass_entry_cb, None)

        self.progress_dialog = MSD.MSDProgressWindow()

        self.change_bar_status(MobileManager.PPP_STATUS_DISCONNECTED)
        
        self.connect_to_bus()
        gobject.timeout_add(2000,self.__initial_actions)
            
        
        self.main_connect_button.connect("clicked", self.__main_connect_button_cb, None)
        self.main_disconnect_button.connect("clicked", self.__main_disconnect_button_cb, None)

    def __initial_actions(self):
        print _(u"begin status")
        #FIXED : Dialer (Async)
        print _(u"Aqui iba un status")
        #self.ppp_manager.status(r_handler_func=self.__async_init_status_cb, e_handler_func=self.__async_init_status_error_cb)
        print _(u"end estatus")
    
    def __async_init_status_cb(self, data):
        #FIXED : Dialer
        if data == MobileManager.PPP_STATUS_CONNECTED:
            self.ppp_manager.stop()
        
    def __async_init_status_error_cb(self, data=None):
        print _(u"error in MSDConnectionsManager in async status")
    
        
    def connect_to_bus (self):
        if self.ppp_manager != None:
            return True

        #FIXED : Dialer
        if self.mcontroller.dialer != None :
            # signal
            self.ppp_manager = self.mcontroller.dialer
            self.ppp_manager.connect("connected", self.__connected_cb)
            self.ppp_manager.connect("disconnected", self.__disconnected_cb)
            self.ppp_manager.connect("connecting", self.__connecting_cb)
            self.ppp_manager.connect("disconnecting", self.__disconnecting_cb)
            
            return True
        else:
            print _(u"No se ha encontrado el ppp manager")
            self.ppp_manager = None
            dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
            dlg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
            dlg.set_markup(MSD.MSG_CONN_MANAGER_NO_PPP_MANAGER_TITLE)
            dlg.format_secondary_markup(MSD.MSG_CONN_MANAGER_NO_PPP_MANAGER)
            dlg.run()
            dlg.destroy()
            return False

    def __ask_pass_entry_cb (self, editable, data):
        if len (self.ask_pass_entry.get_text()) > 0:
            self.ask_pass_ok_button.set_sensitive(True)
        else:
            self.ask_pass_ok_button.set_sensitive(False)  

    def get_connection_params(self, conn):
        if conn == None:
            conn_name = self.conf.get_default_connection_name()
        else:
            conn_name = conn
            
        conn_info = self.conf.get_connection_info_dict(conn_name)
        params = self.conf.get_connection_params(conn_name)
        
        if conn_info["ask_password"] == True:
            show_progress_after = False
            if self.progress_dialog.is_show == True:
                show_progress_after = True
                self.progress_dialog.hide()
            
            self.ask_pass_entry.set_text("")
            self.ask_pass_ok_button.set_sensitive(False)
            self.ask_pass_label.set_markup(MSD.MSG_CONN_MANAGER_ASK_PASSWORD % conn_name)
            
            ret = self.ask_pass_dialog.run()
            if ret == gtk.RESPONSE_OK :
                params[PPP_PARAM_PASSWORD_KEY] = self.ask_pass_entry.get_text()
                self.ask_pass_dialog.hide()
                print "PARAMS : %s" % params
                if show_progress_after == True:
                    self.progress_dialog.show()
                return params
            else:
                self.ask_pass_dialog.hide()
                return None
        else:
            print "PARAMS : %s" % params
            return params
        
    # Return :
    #   - True if everything is ok
    #   - False if the connection failed and need to show a error dialog
    #   - None if the connection failed but not need to show a error dialog
    def connect_to_connection(self, connection_name=None, force_connection=False,
                              action=None, bookmark_info=None):
        if self.abort_now_flag == True:
            self.abort_now_flag = False
            return None
        
        if self.__is_device_selected() == False:
            return None
        #FIXED : Capabilities
        dev = self.mcontroller.get_active_device()
        odev = self.mcontroller.get_device_obj_from_path(dev)
        
        if not odev.has_capability(MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI) :
            self.cardmanager = None
        else:
            self.cardmanager = odev
                
        if action != None:
            self.action = action

        if bookmark_info != None:
            self.bookmark_info = bookmark_info
        
        if self.connect_to_bus() == True:
            #FIXED : Dialer
            ppp_status = self.ppp_manager.status()
            
            #FIXED : Controller (get_card_status)
            if self.cardmanager != None:
                if ppp_status == MobileManager.PPP_STATUS_DISCONNECTED:
                    if self.cardmanager.get_card_status() != MobileManager.CARD_STATUS_READY :
                        dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
                        dlg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
                        dlg.set_markup(MSD.MSG_CONN_MANAGER_NO_CARDMANAGER_TITLE)
                        dlg.format_secondary_markup(MSD.MSG_CONN_MANAGER_NO_CARDMANAGER)
                        dlg.run()
                        dlg.destroy()
                        return None
            
            if ppp_status == MobileManager.PPP_STATUS_CONNECTED :
                #Here if the ppp_manager is working with another connection
                if self.actual_connection == None:
                    #Here if the ppp_manager hasn't reference of the connection's name
                    #For example if the UI crash and the daemon continue running

                    # 1 or <0 => Cancel or close
                    # 2       => Use active connection
                    # 3       => Start new connection
                    if force_connection == False:
                        ret = self.__detect_active_connection_dialog(connection_name, action, bookmark_info)
                    else:
                        ret = 3

                    if ret == 1 or ret < 0:
                        return None

                    if ret == 2 :
                        self.launch_service()
                        return True

                    if ret == 3:
                        print _(u"stop connecction 0")
                        #FIXED : Dialer
                        self.ppp_manager.stop()
                        if connection_name == None:
                            self.waiting_connection = self.conf.get_default_connection_name()
                        else:
                            self.waiting_connection = connection_name

                else:
                    #Here we know the name of the last connection and we'll ask to the user
                    #if he/she wants to change the connection

                    if connection_name == self.actual_connection or ( connection_name == None and self.actual_connection == self.conf.get_default_connection_name()):
                        #it means that the connection is already running
                        self.launch_service()
                        return True

                    if connection_name == None or connection_name == self.conf.get_default_connection_name():
                        #It means that the user wants connect to default connection from old connection

                        # 1 or <0 => Cancel or close
                        # 2       => Use active connection
                        # 3       => Start new connection
                        if force_connection == False:
                            ret = self.__detect_active_connection_dialog(self.conf.get_default_connection_name(), action, bookmark_info)
                        else:
                            ret = 3
                        
                        if ret == 1 or  ret < 0 :
                            return None

                        if ret == 2 :
                            self.launch_service()
                            return True

                        if ret == 3:
                            print _(u"stop connecction 1")
                            #FIXED : Dialer
                            self.ppp_manager.stop()
                            if connection_name == None:
                                self.waiting_connection = self.conf.get_default_connection_name()
                            else:
                                self.waiting_connection = connection_name
                            return True
                    else:
                        #It means that the user wants connect to new connection from old connection

                        # 1 or <0 => Cancel or close
                        # 2       => Use active connection
                        # 3       => Start new connection
                        if force_connection == False:
                            ret = self.__detect_active_connection_dialog(connection_name, action, bookmark_info)
                        else:
                            ret = 3
                        
                        if ret == 1 or  ret < 0 :
                            return None

                        if ret == 2 :
                            self.launch_service()
                            return True

                        if ret == 3:
                            print _(u"stop connection 2")
                            #FIXED : Dialer
                            self.ppp_manager.stop()
                            if connection_name == None:
                                self.waiting_connection = self.conf.get_default_connection_name()
                            else:
                                self.waiting_connection = connection_name
                            return True
                        
            else:
                #Here if there isn't a connection stablished

                # 1 or <0 => Cancel or Close
                # 2       => Connect to new connection

                if force_connection == False:
                    ret = self.__new_connection_active_dialog(connection_name, action, bookmark_info)
                else:
                    ret = 2
                
                if ret == 1 or  ret < 0:
                    return None
                
                if ret == 2 :
                    print _(u"start connection")
                    #if self.cardmanager != None:
                    #    self.cardmanager.stop_polling()
                        
                    params = self.get_connection_params(connection_name)
                    if params == None:
                        return None
                    #FIXED : Dialer (start !!)
                    self.ppp_manager.start(params)
                    if connection_name == None:
                        self.starting_connection = self.conf.get_default_connection_name()
                    else:
                        self.starting_connection = connection_name
                    return True

        return True

    def disconnect(self):
        print _(u"disconnect")
        self.connect_with_phone = None
        #FIXED : Dialer
        self.ppp_manager.stop()
    
    def close_app(self):
	if self.ppp_manager == None:
            return True
        #FIXED : Dialer
        if self.ppp_manager.status() == MobileManager.PPP_STATUS_CONNECTED:
            dlg = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_INFO)
            dlg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
            dlg.set_markup (MSD.MSG_CONN_MANAGER_APP_CLOSE_TITLE)
            dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_APP_CLOSE)
            dlg.add_buttons(gtk.STOCK_CANCEL, 1, gtk.STOCK_OK, 2)
            ret = dlg.run()
            dlg.destroy()

            if ret == 1 or ret < 0:
                return False
            else:
                #FIXED : Dialer
                print "force disconnect"
                if self.main_window.consum_manager.running == True:
                    self.main_window.consum_manager.end_history()
                    self.main_window.consum_manager.running = False
                    
                self.ppp_manager.stop()
                
                return True
        else:
            return True

    def __is_device_selected (self):
        print _(u"MSDConnManager : method -> __is_device_selected()")
        #FIXED: Controller (Esto devuelve None)
        dev = self.mcontroller.get_active_device()
	if dev != None :
		return True
	else:
		return False

    def __main_connect_button_cb(self, widget, data):        
        ret = self.connect_to_connection()
        if ret == False:
            self.error_on_connection()

    def error_on_connection(self):
        self.progress_dialog.hide()
        dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
        dlg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        dlg.set_markup(MSD.MSG_CONN_MANAGER_CONNECTION_ERROR_TITLE)
        dlg.format_secondary_markup(MSD.MSG_CONN_MANAGER_CONNECTION_ERROR)
        dlg.run()
        dlg.destroy()

    def __main_disconnect_button_cb(self, widget, data):
        self.disconnect()

    def launch_service(self):
        if self.action != None:
            self.action.launch_action()
            self.action = None

        if self.bookmark_info != None:
            print "launch gnome-open %s" % self.bookmark_info[1]
            ret = os.system("gnome-open %s" % self.bookmark_info[1])
            
            if ret != 0:
                if os.path.exists(self.bookmark_info[1].replace("file://","")):
                    os.spawnle (os.P_NOWAIT, "%s" % self.bookmark_info[1].replace("file://",""), "", os.environ)

            self.bookmark_info = None

    def change_bar_status(self,new_status=None):
        if new_status is None:
            #FIXED : Dialer
            status = self.ppp_manager.status()
        else:            
            status = new_status

        
        print _(u"actualizando ui para %s") % status

        if status == MobileManager.PPP_STATUS_CONNECTED :
            self.connect_statusbar_im.show()
            self.disconnect_statusbar_im.hide()
            
        if status == MobileManager.PPP_STATUS_DISCONNECTED :
            self.connect_statusbar_im.hide()
            self.disconnect_statusbar_im.show()

        ctx = self.status_bar.get_context_id("contexto de conexiones")
        self.status_bar.pop(ctx)
        if status != MobileManager.PPP_STATUS_CONNECTED:
            self.status_bar.push(ctx, STATUS_MESSAGES[status])
        else:
            msg = "%s" % STATUS_MESSAGES[status]
            self.status_bar.push(ctx, msg)

    def __set_sensitive_connect_disconnect_buttons(self, value):
        self.main_disconnect_button.set_sensitive(value)
        self.main_connect_button.set_sensitive(value)

    def __abort_connection_now(self):
        if self.starting_connection != None:
            #FIXED : Dialer
            self.ppp_manager.stop()
        self.abort_now_flag = True

    def __abort_connection_actions(self):
        self.action = None
        self.bookmark_info = None
        self.actual_connection = None
        self.starting_connection = None
        self.connection_successful = False
        self.waiting_connection = None
        #self.abort_now_flag = False
        self.main_disconnect_button.hide()
        self.main_connect_button.show()
        self.__set_sensitive_connect_disconnect_buttons(True)
        self.change_bar_status(new_status=MobileManager.PPP_STATUS_DISCONNECTED)
        self.main_stats.set_connection_name(value=_(u"Ninguna establecida"))

    def __connected_cb(self, dialer):
        self.progress_dialog.set_show_buttons(True)
        self.progress_dialog.hide()
        if self.abort_now_flag == True:
            self.__abort_connection_actions()
            print _(u"CONNECTED (Aborted)") 
            return
            
        self.main_disconnect_button.show()
        self.main_connect_button.hide()
        self.__set_sensitive_connect_disconnect_buttons(True)

        self.actual_connection = self.starting_connection
        self.starting_connection = None
        self.launch_service()
        print _(u"CONNECTED ---> %s") % self.actual_connection
        self.main_stats.set_connection_name(self.actual_connection)
        self.connection_successful = True

    def __connecting_cb(self, dialer):
        self.progress_dialog.set_show_buttons(True)
        if self.abort_now_flag == True:
            self.progress_dialog.hide()
            self.__abort_connection_actions()
            self.abort_now_flag == False
            self.reconnecting_flag = False
            self.connection_successful = False
            print _(u"CONNECTING (Aborted)")
            return
        
        self.__set_sensitive_connect_disconnect_buttons(False)
        if self.reconnecting_flag == False:
            self.progress_dialog.show(title=None, message=_(u"Conectando con '%s'") %  self.starting_connection,
                                      cancel_callback=self.__abort_connection_now)
        print _(u"CONNECTING ---> %s") % self.starting_connection
        self.connection_successful = False
        self.reconnecting_flag = False

    def __disconnected_cb(self, dialer):
        self.progress_dialog.set_show_buttons(False)
        if self.connection_successful == False:
            if self.abort_now_flag == True:
                self.progress_dialog.hide()
                self.__abort_connection_actions()
                self.abort_now_flag == False
                print _(u"DISCONNECTED (Aborted)")
                return
            
            self.error_on_connection()
            self.actual_connection = None
            self.waiting_connection = None
            self.connection_successful = None
            self.main_disconnect_button.hide()
            self.main_connect_button.show()
            self.main_connect_button.set_sensitive(True)
        else:
            self.main_disconnect_button.hide()
            self.main_connect_button.show()
            self.__set_sensitive_connect_disconnect_buttons(True)
            #if self.cardmanager != None:
            #    self.cardmanager.start_polling()
        
            self.main_stats.set_connection_name()
            if self.waiting_connection == None:
                self.progress_dialog.hide()
                self.actual_connection = None
                if self.abort_now_flag == True:
                    self.__abort_connection_actions()
                    self.abort_now_flag == False
                    print _(u"DISCONNECTED (Aborted)")
                    return
            else:
                self.actual_connection = None
                conn = self.waiting_connection
                self.waiting_connection = None
                if self.abort_now_flag == True:
                    self.progress_dialog.hide()
                    self.__abort_connection_actions()
                    self.abort_now_flag == False
                    self.reconnecting_flag = False
                    print _(u"DISCONNECTED (Aborted)")
                    return
                
                if self.connect_to_connection(connection_name=conn, force_connection=True, action=self.action, bookmark_info=self.bookmark_info) == False:
                    if self.abort_now_flag == False :
                        self.error_on_connection()
                    else:
                        self.progress_dialog.hide()
                        self.__abort_connection_actions()
                        self.abort_now_flag == False
                        self.reconnecting_flag = False
                        print _(u"DISCONNECTED (Aborted)")
                        return
        
        self.connect_with_phone = None
        print _(u"DISCONNECTED ---> %s") % self.actual_connection
        
    def __disconnecting_cb(self, dialer):
        self.progress_dialog.set_show_buttons(False)
        if self.connection_successful == True:
            self.__set_sensitive_connect_disconnect_buttons(False)
            if self.waiting_connection == None:
                self.progress_dialog.show(title=None, message=_(u"Desconectando '%s'") %  self.actual_connection,
                                          cancel_callback=self.__abort_connection_now)
            else:
                self.progress_dialog.set_show_buttons(True)
                self.progress_dialog.show(title=None, message=_(u"Conectando a '%s'") %  self.waiting_connection,
                                          cancel_callback=self.__abort_connection_now)
                self.reconnecting_flag = True
        else:
            self.progress_dialog.hide()
        
        print _(u"DISCONNECTING ---> %s") % self.actual_connection
        
    def __detect_active_connection_dialog(self, connection_name, action, bookmark_info):
        if self.conf.conf["connections_general"]["ask_before_change_connection"] == False:
            return 3
            
        dlg = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_INFO)
        dlg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        dlg.set_markup (MSD.MSG_CONN_MANAGER_ACTIVE_CONN_DETECT_TITLE)
        vbox = dlg.vbox
        check_button = gtk.CheckButton(_(u"No volver a mostrar este dialogo"))
        vbox.pack_end(check_button, expand=False)
        check_button.hide()
        
        if connection_name == None or connection_name == self.conf.get_default_connection_name():
            if action != None:
                dlg.set_markup (MSD.MSG_CONN_MANAGER_OPEN_SERVICE_TITLE % action.get_conf_key_value("name"))
                dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_OPEN_SERVICE_WITH_ACTIVE_CONN_DEFAULT % (self.actual_connection, self.conf.get_default_connection_name()))
                
            if bookmark_info != None:
                dlg.set_markup (MSD.MSG_CONN_MANAGER_OPEN_BOOKMARK_TITLE % bookmark_info[0])
                dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_OPEN_BOOKMARK_WITH_ACTIVE_CONN_DEFAULT  % (self.actual_connection, self.conf.get_default_connection_name()))
                
            if action == None and bookmark_info == None:
                dlg.set_markup(MSD.MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_TITLE)
                dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_WITH_ACTIVE_CONN % (self.actual_connection, self.conf.get_default_connection_name()))
        else:
            if action != None:
                dlg.set_markup (MSD.MSG_CONN_MANAGER_OPEN_SERVICE_TITLE % action.get_conf_key_value("name"))
                dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_OPEN_SERVICE_WITH_ACTIVE_CONN % (self.actual_connection, connection_name))
            if bookmark_info != None:
                dlg.set_markup (MSD.MSG_CONN_MANAGER_OPEN_BOOKMARK_TITLE % bookmark_info[0])
                dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_OPEN_BOOKMARK_WITH_ACTIVE_CONN % (self.actual_connection, connection_name))

        dlg.add_buttons(gtk.STOCK_CANCEL, 1, _(u"_Usar conexión establecida"), 2, gtk.STOCK_CONNECT, 3)                  
        ret = dlg.run()
        dlg.destroy()
        
        if ret > 1 :
            self.conf.conf["connections_general"]["ask_before_change_connection"] = not check_button.get_active()
            self.conf.save_conf()
        
        return ret

    def __new_connection_active_dialog(self, connection_name, action, bookmark_info):
        if action == None and bookmark_info == None:
            if self.conf.conf["connections_general"]["ask_before_connect"] == False:
                return 2
        else:
            if self.conf.conf["connections_general"]["ask_before_connect_to_action"] == False:
                return 2
        
        dlg = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_INFO)
        dlg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        vbox = dlg.vbox
        check_button = gtk.CheckButton(_(u"No preguntar de nuevo"))
        vbox.pack_end(check_button, expand=False)
        check_button.set_property("has-focus", False)
        check_button.set_property("has-default", False)
        check_button.set_property("can-focus", False)
        check_button.set_property("can-default", False)
        check_button.show()

        if connection_name == None:
            if action != None:
                dlg.set_markup (MSD.MSG_CONN_MANAGER_OPEN_SERVICE_TITLE % action.get_conf_key_value("name"))
                
            if bookmark_info != None:
                dlg.set_markup (MSD.MSG_CONN_MANAGER_OPEN_BOOKMARK_TITLE % bookmark_info[0])
                
            if action == None and bookmark_info == None:
                dlg.set_markup(MSD.MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_TITLE)

            dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_CONN % self.conf.get_default_connection_name())
        else:
            if action != None:
                dlg.set_markup (MSD.MSG_CONN_MANAGER_OPEN_SERVICE_TITLE  % action.get_conf_key_value("name"))
                dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_OPEN_SERVICE  % connection_name)
            if bookmark_info != None:
                dlg.set_markup (MSD.MSG_CONN_MANAGER_OPEN_BOOKMARK_TITLE % bookmark_info[0])
                dlg.format_secondary_markup (MSD.MSG_CONN_MANAGER_OPEN_BOOKMARK % connection_name)

        dlg.add_buttons(gtk.STOCK_CANCEL, 1, gtk.STOCK_CONNECT, 2)
        dlg.set_default_response(2)
        ret = dlg.run()
        dlg.destroy()

        if action == None and bookmark_info == None:
            if ret > 1:
                self.conf.conf["connections_general"]["ask_before_connect"] = not check_button.get_active()
                self.conf.save_conf()
        else:
            if ret > 1:
                self.conf.conf["connections_general"]["ask_before_connect_to_action"] = not check_button.get_active()
                self.conf.save_conf()
        
        return ret
