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
import pango
import gnome
import MSD
import dbus
import dbus.glib
import gobject
import PPPManager
import os
import sys

import MobileManager
import MobileManager.ui

import MSDAddressBook
from MSDUtils import *

# TODO: localizarlo en un fichero

class MSDMainWindow:
    def __init__(self):
        self.xml = gtk.glade.XML(MSD.glade_files_dir + "mainwindow.glade")
        self.main_window = self.xml.get_widget("main_window")
        self.main_window.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        self.main_vbox = self.xml.get_widget("main_vbox")
        self.mcontroller = MobileManager.MobileController()
        self.main_toolbar = MSD.MSDMainToolbar()
        self.main_stats = MSD.MSDMainStatsWidget(self.mcontroller)
        self.main_vbox.pack_start(self.main_toolbar)
        self.main_vbox.reorder_child(self.main_toolbar, 0)
        self.main_vbox.pack_start(self.main_stats)
        self.main_vbox.reorder_child(self.main_stats, 3)
        self.actions_scrolled_window = self.xml.get_widget("actions_scrolled_window")
        self.main_actions_view = self.xml.get_widget("main_actions_view")
        self.bookmarks_button = self.main_toolbar.bookmarks_button
        self.preferences_button = self.main_toolbar.preferences_button
        self.addressbook_button = self.main_toolbar.addressbook_button
        self.help_button = self.main_toolbar.help_button
        self.expander_eventbox = self.xml.get_widget("expander_eventbox")
        self.expander_eventbox.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#ecf1fb"))
        self.main_separator = self.xml.get_widget("main_separator")
        self.mini_banner_eventbox = self.xml.get_widget("mini_banner_eventbox")
        self.mini_banner_eventbox.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#525774"))
        self.mini_banner_image =  self.xml.get_widget("mini_banner_image")
        self.mini_banner_image.set_from_file(MSD.icons_files_dir + "mini_banner.png")
        self.device_eventbox = self.xml.get_widget("device_eventbox")
        self.device_eventbox.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#515774"))
        self.main_statusbar = self.xml.get_widget("main_statusbar")
        self.connect_statusbar_im = gtk.Image()
        self.connect_statusbar_im.set_from_file(MSD.icons_files_dir + "conectado.png")
        self.disconnect_statusbar_im = gtk.Image()
        self.disconnect_statusbar_im.set_from_file(MSD.icons_files_dir + "no-conectado.png")
        self.main_statusbar.pack_start(self.connect_statusbar_im, expand=False)
        self.main_statusbar.pack_start(self.disconnect_statusbar_im, expand=False)
        self.connect_statusbar_im.hide()
        self.disconnect_statusbar_im.hide()
        self.main_statusbar.reorder_child(self.connect_statusbar_im, 0)
        self.main_statusbar.reorder_child(self.disconnect_statusbar_im, 1)
        self.card_statusbar = gtk.Statusbar()
        self.card_statusbar.set_has_resize_grip(False)
        self.main_statusbar.pack_end(self.card_statusbar, expand=True)
        self.card_statusbar.hide()
        
        self.consum_button = self.main_stats.consum_button
        self.connect_button = self.main_stats.connect_button
        self.disconnect_button = self.main_stats.disconnect_button
        self.expander_on_button = self.xml.get_widget("expander_on_button")
        self.expander_off_button = self.xml.get_widget("expander_off_button")
        self.device_selected_button = self.xml.get_widget("device_selected_button")
        self.device_selected_label = self.xml.get_widget("device_selected_label")
        self.device_selected_button.modify_bg(gtk.STATE_PRELIGHT, gtk.gdk.color_parse("#717aa6"))
        self.device_selected_button.modify_bg(gtk.STATE_ACTIVE, gtk.gdk.color_parse("#717aa6"))
        self.device_selected_button.modify_bg(gtk.STATE_SELECTED, gtk.gdk.color_parse("#717aa6"))
        self.at_options_button = MobileManager.ui.MobileATOptionsButton(self.mcontroller)

        dev = self.mcontroller.get_active_device()
        if dev == None :
            self.device_selected_label.set_markup(u'<span foreground="white">Ningún dispositivo disponible</span>')
        else:
            self.device_selected_label.set_markup ('<span foreground="white">%s</span>' % dev.get_property("pretty-name"))

        
        self.can_show_main_pcmcia_menu = False

        self.MSDConf = MSD.MSDConf()

        self.MSDConnManager = MSD.MSDConnectionsManager(self.MSDConf, self)
        
        self.MSDActManager = MSD.MSDActionsManager(self.MSDConf, self.MSDConnManager)

        liststore = self.MSDActManager.get_actions_model()
        cellrenderpixbuf = gtk.CellRendererPixbuf()
        cellrenderpixbuf.set_property('cell-background', '#DEDEDE')
        cellrendertext = gtk.CellRendererText()
        cellrendertext.set_property('cell-background', '#DEDEDE')
        
        column = gtk.TreeViewColumn('MAV_icon', cellrenderpixbuf,
                                    pixbuf=0, cell_background_set=3)
        self.main_actions_view.append_column(column)
        column = gtk.TreeViewColumn('MAV_name', cellrendertext,
                                    markup=1, cell_background_set=3)
        self.main_actions_view.append_column(column)

        # assign the tooltip
        tips = gtk.Tooltips()
        tips.set_tip(self.main_actions_view, "")

        self.main_actions_view.set_model(liststore)
        
        self.preferences = MSD.MSDPreferencesDialog(self)

        self.MSDActManager.set_connections_model(self.preferences.get_connections_model())


        # Consum
        self.consum = MSD.MSDConsumWindow(self.MSDConf)
        self.consum_manager = MSD.MSDConsumManager(self.mcontroller, self.main_stats, self.consum,self.MSDConf)

        # Help stuff

        self.progress_dialog = MSD.MSDProgressWindow()
        self.progress_dialog.set_show_buttons(False)
        self.timer_id = 0

        self.help_dialog = self.xml.get_widget("help_window")
        self.help_dialog.set_icon_from_file(MSD.icons_files_dir + "ayuda_16x16.png")
        self.hd_help_button = self.xml.get_widget("help_button")
        self.hd_help_online_button = self.xml.get_widget("help_online_button")
        self.hd_help_close_button = self.xml.get_widget("help_close_button")
        self.hd_help_info_treeview = self.xml.get_widget("help_info_treeview")
        self.help_liststore = gtk.ListStore(str, str)
        column = gtk.TreeViewColumn('Option_key', gtk.CellRendererText(),
                                    markup=0)
        self.hd_help_info_treeview.append_column(column)
        column = gtk.TreeViewColumn('Option_value', gtk.CellRendererText(),
                                    markup=1)
        self.hd_help_info_treeview.append_column(column)
        self.hd_help_info_treeview.set_model(self.help_liststore)

        # Bookmarks
        self.bookmarks_menu = gtk.Menu()
        for bookmark in self.MSDConf.get_bookmarks_list():
            self.bookmarks_menu.append(gtk.MenuItem(gobject.markup_escape_text(bookmark[0]), False))
        
        #Warning
        self.warning_dialog = MSD.MSDWarningDialog(self.xml, self.main_window, self.MSDConf)

        # Main actions menu
        self.main_actions_menu = gtk.Menu()
        self.main_actions_menu_item_mail = gtk.MenuItem(u"Nuevo correo")
        self.main_actions_menu_item_open = gtk.ImageMenuItem("Abrir")
        self.main_actions_menu_item_config = gtk.MenuItem(u"Configuración")
        self.main_actions_menu_item_install = gtk.MenuItem("Instalar")
        self.main_actions_menu_item_help = gtk.ImageMenuItem(gtk.STOCK_HELP)
        self.main_actions_menu.append (self.main_actions_menu_item_mail)
        self.main_actions_menu.append (self.main_actions_menu_item_open)
        self.main_actions_menu.append (self.main_actions_menu_item_config)
        self.main_actions_menu.append (self.main_actions_menu_item_install)
        self.main_actions_menu.append (self.main_actions_menu_item_help)

        #Signals !
        self.MSDActManager.connect_signals()
        self.main_window.connect("delete-event", self.__closeapp_cb, None)
        self.bookmarks_button.connect("clicked", self.__show_bookmarks_cb, None)
        self.addressbook_button.connect("clicked",self.__show_addressbook_cb,None)
        self.preferences_button.connect("clicked", self.__show_preferences_cb, None)
        self.help_button.connect("clicked", self.__show_help_cb, None)
        self.hd_help_button.connect("clicked", self.__open_help_button_cb, None)
        self.hd_help_online_button.connect("clicked", self.__open_help_online_button_cb, None)
        self.hd_help_close_button.connect("clicked", self.__close_help_dialog_cb, None)
        self.help_dialog.connect("delete_event", self.__close_help_dialog_cb)
        self.expander_on_button.connect("clicked", self.__expander_on_button_cb, None)
        self.expander_off_button.connect("clicked", self.__expander_off_button_cb, None)
        self.consum_button.connect("clicked", self.__consum_button_cb)
        self.device_selected_button.connect("clicked", self.__device_selected_button_cb, None)
        self.main_actions_view.connect("row-activated", self.__action_activated_cb, None)
        self.main_actions_view.connect("event", self.__roll_over_main_actions_treeview_cb, None)
        self.main_actions_view.connect("motion-notify-event", self.__main_actions_view_tooltip, tips, 4)
        self.main_actions_view.set_events(gtk.gdk.POINTER_MOTION_MASK)
        self.main_actions_menu_item_mail.connect("activate", self.__main_actions_menu_item_mail_cb, None)
        self.main_actions_menu_item_open.connect("activate", self.__main_actions_menu_item_open_cb, None)
        self.main_actions_menu_item_config.connect("activate", self.__main_actions_menu_item_config_cb, None)
        self.main_actions_menu_item_install.connect("activate", self.__main_actions_menu_item_install_cb, None)
        self.main_actions_menu_item_help.connect("activate", self.__main_actions_menu_item_help_cb, None)

        self.mcontroller.connect ("active-device-changed", self.__active_device_changed_cb)
        self.mcontroller.connect ("active-dev-card-status-changed", self.__active_device_card_status_changed)
        self.mcontroller.connect ("added-device", self.__device_added_cb)
        self.mcontroller.connect ("removed-device", self.__device_removed_cb)
        self.mcontroller.dialer.connect("connected", self.__connected_cb)
        self.mcontroller.dialer.connect("disconnected", self.__disconnected_cb)
        self.mcontroller.dialer.connect("connecting", self.__connecting_cb)
        self.mcontroller.dialer.connect("disconnecting", self.__disconnecting_cb)
        
        self.pcmcia_manager = None

        # Systray
        self.systray = MSD.MSDSystray(self.MSDConf, self.main_window, self.MSDConnManager, self.MSDActManager, self.mcontroller)
        self.MSDActManager.set_systray(self.systray)

        if self.MSDConf.get_ui_general_key_value ("systray_showing_mw"):
            self.main_window.show()
        else:
            self.main_window.hide()

        self.main_stats.show()
        self.main_toolbar.show()
        
        if self.MSDConf.get_ui_general_key_value("main_expander_on") == True:
            self.__expander_on_button_cb(None, None)
        else:
            self.__expander_off_button_cb(None, None)
        
        # pongo un timeout para ejecutar las acciones que deben se ejecutadas de manera autonoma (sin intervencion del usuario)
        # al cargarse la aplicacion
        gobject.timeout_add(1,self.__initial_actions_cb)


        if len(sys.argv) == 3 :
            if os.path.exists(sys.argv[2]):
                 self.preferences.importer.import_from_file(sys.argv[2])

        gobject.timeout_add(3000, self.__import_while_app_working_cb)
        
        # Addressbook
        self._addressbook_controller = None
        self._user_book = MSDAddressBook.get_user_addressbook()
        self._select_contact_window = self.xml.get_widget("select_contact_window")
        self._select_contact_window.set_icon_from_file(MSD.icons_files_dir + "mail_16x16.png")
        self._sc_send_mail_button = self.xml.get_widget("sc_send_mail_button")
        self._sc_cancel_button = self.xml.get_widget("sc_cancel_button")
        ## treeview
        self._sc_treeview = self.xml.get_widget("sc_contacts_treeview")
        base_id = 1
        for field in ["Nombre",u"Correo electrónico"]:
            col = gtk.TreeViewColumn(field)
            self._sc_treeview.append_column(col)
            cell = gtk.CellRendererText()
            col.pack_start(cell, True)
            col.add_attribute(cell, 'text', base_id)
            col.set_resizable(True)
            cell.set_property("ellipsize", pango.ELLIPSIZE_END)
            base_id = base_id + 1
            
        model = gtk.ListStore(gobject.TYPE_INT,
                               gobject.TYPE_STRING,
                               gobject.TYPE_STRING)
        self._sc_treeview.set_model(model)
        #signals
        self._select_contact_window.connect("delete-event", self.__sl_delete_cb, None)
        self._sc_send_mail_button.connect("clicked", self.__sl_send_mail_cb, None)
        self._sc_cancel_button.connect("clicked", self.__sl_delete_cb, None)

        # Select active device if available
        self.__select_active_device_on_init()

        # Feed updater
        self.update_checker = MSD.MSDUpdateChecker (self.MSDConf, self.mcontroller)

    def __sl_delete_cb(self, widget, event, data=None):
        self._select_contact_window.hide()
        return True

    def __sl_send_mail_cb(self, widget, event, data=None):
        selection = self._sc_treeview.get_selection()

        if selection.count_selected_rows()!= 1:
            return None
      
        model, itera  = selection.get_selected()
        contact_id = model.get_value(itera,0)
        contact  = MSDAddressBook.MDContact.get(contact_id)
        if contact is not None:
            gnome.url_show("mailto:%s" % contact.email)
            
        self._select_contact_window.hide()

    def __main_actions_menu_item_mail_cb (self, menu_item, data):
        if self._user_book is None:
            return None
        model = self._sc_treeview.get_model()
        model.clear()
        for contact in self._user_book.get_all_with_mail():
            model.append([contact.id ,
                          contact.name,
                          contact.email])
        self._select_contact_window.show_all()

    def __select_active_device_on_init(self):
        if os.path.exists(MSD.default_device_file) :
            fd = open(MSD.default_device_file)
            default_udi = fd.readline().strip("\n")
            for dev in  self.mcontroller.get_available_devices() :
                if dev.dev_props["info.udi"] == default_udi :
                    self.mcontroller.set_active_device(dev.dev_props["info.udi"])
                    print "Set device from device default file -> %s" % dev.dev_props["info.udi"]
                    return
                
        if len(self.mcontroller.get_available_devices()) > 0 :
            dev_list = self.mcontroller.get_available_devices()
            self.mcontroller.set_active_device(dev_list[0].dev_props["info.udi"])
        else:
            self.mcontroller.set_active_device(None)
        
    def __main_actions_view_tooltip (self, widget, e, tooltips, cell, emptyText=""):
        try:
            (path,col,x,y) = widget.get_path_at_pos(int(e.x), int(e.y))
        except:
            return
        it = widget.get_model().get_iter(path)
        value = widget.get_model().get_value(it, cell)
        tooltips.set_tip(widget, value)
        tooltips.enable()

    def __roll_over_main_actions_treeview_cb (self, widget, event, data):
        if event.type == gtk.gdk.MOTION_NOTIFY :
            coords = event.get_coords()
            path_at_pos = self.main_actions_view.get_path_at_pos(int(coords[0]), int(coords[1]))
            if path_at_pos != None:
                selection = self.main_actions_view.get_selection()
                model = self.main_actions_view.get_model()
                tmp_iter = model.get_iter(path_at_pos[0])
                codename = model.get_value(tmp_iter, 2)
                installed_value = self.MSDConf.get_action_key_value(codename, "installed")
                if installed_value == True:
                    selection.select_path(path_at_pos[0])

        if event.type == gtk.gdk.BUTTON_PRESS:
            if event.button == 3:
                coords = event.get_coords()
                path_at_pos = self.main_actions_view.get_path_at_pos(int(coords[0]), int(coords[1]))
                if path_at_pos != None:
                    model = self.main_actions_view.get_model()
                    tmp_iter = model.get_iter(path_at_pos[0])
                    codename = model.get_value(tmp_iter, 2)
                    installed_value = self.MSDConf.get_action_key_value(codename, "installed")
                    if installed_value == True:
                        self.main_actions_menu_item_open.set_sensitive(True)
                        self.main_actions_menu_item_config.set_sensitive(True)
                        self.main_actions_menu_item_install.set_sensitive(False)
                    else:
                        self.main_actions_menu_item_open.set_sensitive(False)
                        self.main_actions_menu_item_config.set_sensitive(False)
                        self.main_actions_menu_item_install.set_sensitive(True)
                    self.main_actions_menu.popup(None, None, None, event.button, event.time)
                    self.main_actions_menu.show_all()
                    if (codename == "MSDASendMMS"):
                        self.main_actions_menu_item_mail.show()
                    else:
                        self.main_actions_menu_item_mail.hide()
        
    def __main_actions_menu_item_open_cb (self, menu_item, data):
        (model, iter) = self.main_actions_view.get_selection().get_selected()
        action_codename = model.get_value(iter, 2)
        self.MSDActManager.launch_action(action_codename)

    def __main_actions_menu_item_config_cb (self, menu_item, data):
        (model, iter) = self.main_actions_view.get_selection().get_selected()
        codename = model.get_value(iter, 2)
        selected_service = self.preferences.get_actions_path_from_codename (codename)
        self.preferences.show_all(tab=selected_service)

    def __main_actions_menu_item_install_cb (self, menu_item, data):
        (model, iter) = self.main_actions_view.get_selection().get_selected()
        action_codename = model.get_value(iter, 2)
        self.progress_dialog.show("Instalando servicio", "Por favor, espera mientras se instala el servicio '%s'." % self.MSDConf.get_action_key_value(action_codename, "name"))
        self.timer_id = gobject.timeout_add(2000, self.__progress_timer_cb)
        self.MSDConf.set_action_key_value(action_codename, "installed", True)
        self.MSDActManager.reload_actions_model(clear=True)

    def __main_actions_menu_item_help_cb (self, menu_item, data):
        (model, iter) = self.main_actions_view.get_selection().get_selected()
        action_codename = model.get_value(iter, 2)
        self.MSDActManager.launch_help(action_codename)

    def __progress_timer_cb (self):
        self.progress_dialog.hide()
        
    def __action_activated_cb (self, treeview, path, view_column, data):
        model = treeview.get_model()
        iter = model.get_iter(path)
        action_codename = model.get_value(iter, 2)
        if self.MSDConf.get_action_key_value(action_codename, "installed") == True:
            self.MSDActManager.launch_action(action_codename)

    def __closeapp_cb(self, widget, event, data=None):
        close_app(self.MSDConnManager, self.MSDConf, self.update_checker)
        return True

    def __show_bookmarks_cb(self, widget, data):
        entries = []
        ui_info = ""
        bookmarks_list = self.MSDConf.get_bookmarks_list()
        bookmarks_list.sort(lambda x,y : int(x[4]-y[4]))
        
        entries.append (("add_bookmark", None, "Nuevo acceso directo...", "tooltip"))
        entries.append (("conf_bookmark", None, "Configurar accesos directos...", "tooltip"))
        for x in bookmarks_list :
            entries.append((x[0], gtk.STOCK_OPEN, x[0], None, "tooltip"))
            ui_info = ui_info + "   <menuitem action='%s'/>\n" % gobject.markup_escape_text(x[0])

        extra_info = "<menuitem action='add_bookmark'/><menuitem action='conf_bookmark'/><separator/>\n"
        ui_info ="<ui>\n<popup action='BookmarksMenu'>\n" + extra_info + ui_info + "  </popup>\n</ui>"
        entries = tuple(entries)

        actions = gtk.ActionGroup("Actions")
        actions.add_actions(entries)
        ui = gtk.UIManager()
        ui.insert_action_group(actions, 0)
        ui.add_ui_from_string(unicode(ui_info))

        add_bookmark_item = ui.get_widget("/BookmarksMenu/add_bookmark")
        conf_bookmark_item = ui.get_widget("/BookmarksMenu/conf_bookmark")

        add_bookmark_item.connect ("activate", self.__add_bookmark_item_cb, None)
        conf_bookmark_item.connect ("activate", self.__conf_bookmark_item_cb, None)
        
        self.bookmarks_menu = ui.get_widget("/BookmarksMenu")
        for x in bookmarks_list :
            item = ui.get_widget("/BookmarksMenu/%s" % x[0])
            item.connect("activate", self.__bookmarks_item_cb, x[0])
            image = gtk.Image()
            fav_icon = self.MSDConf.get_bookmark_icon(x[0])
            image.set_from_pixbuf(fav_icon)
            item.set_image(image)
        self.bookmarks_menu.popup(None, None, self.__bookmarks_locate, 0, 0)
        self.bookmarks_menu.show_all()

    def __add_bookmark_item_cb(self, widget, data):
        self.preferences.bookmarks_tab.add_bookmark(True)
        
    def __conf_bookmark_item_cb(self, widget, data):
        self.preferences.show_all(tab=0)

    def __bookmarks_item_cb(self, widget, data):
        connection = self.MSDConf.get_bookmark_connection(data)
        if connection == "":
            connection = None
        url = self.MSDConf.get_bookmark_url(data)

        if connection == None :
            if self.MSDConnManager.ppp_manager.status() == MobileManager.PPP_STATUS_CONNECTED:
                ret = os.system("gnome-open %s" % url)
                if ret != 0:
                    if os.path.exists(url.replace("file://","")):
                        os.spawnle(os.P_NOWAIT, "%s" % url.replace("file://",""), "", os.environ)
            else:
                if self.MSDConnManager.connect_to_connection(connection_name=connection, bookmark_info=[data, url]) == False:
                    self.MSDConnManager.error_on_connection()
        else:
            if self.MSDConnManager.connect_to_connection(connection_name=connection, bookmark_info=[data, url]) == False:
                self.MSDConnManager.error_on_connection()

    def __bookmarks_locate(self, menu):
        button_window = self.bookmarks_button.window
        button_rectangle = self.bookmarks_button.get_allocation()
        x, y = button_window.get_position()
        return (x+button_rectangle.x, y+button_rectangle.height, True)

    def __show_preferences_cb(self, widget, data):
        self.preferences.show_all()

    def __show_addressbook_cb(self,widget,data):
        if self._addressbook_controller is None:
            dlg = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_INFO, gtk.BUTTONS_NONE,"")
            dlg.set_markup("")
            dlg.format_secondary_text("Cargando datos de la agenda...\nPor favor espera. ")
            dlg.show()
            gobject.timeout_add(30,self.__load_addressbook_cb,dlg)
        else:
            self._addressbook_controller.show()

        
    def __load_addressbook_cb(self,dlg=None):
        self._addressbook_controller = MSDAddressBook.get_addressbook_controller(self.MSDConf,self.preferences)
        dlg.destroy()
        self._addressbook_controller.show()
        
    def __show_help_cb(self, widget, data):
        self.help_dialog.show()
        self.help_dialog.deiconify()
        self.hd_help_close_button.grab_focus()
        
    def __open_help_button_cb(self, widget, data):
        os.system("gnome-open %s" % MSD.help_uri)
        
    def __open_help_online_button_cb(self, widget, data):
        os.system("gnome-open %s" % MSD.help_online_uri)

    def __close_help_dialog_cb(self, widget, event):
        self.help_dialog.hide()
        return True

    def __load_help_info(self):
        self.help_liststore.clear()
        dev = self.mcontroller.get_active_device()
        if dev == None :
            self.help_liststore.append([u"Información no disponible",""])
            return
        
        if MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
            info = dev.get_card_info()
            for x in info:
                x_tmp = x.replace("Model","Modelo").replace("Manufacturer","Fabricante").replace("Revision","Firmware")
                if not x.startswith("+") :
                    self.help_liststore.append([x_tmp,""])
        else:
           self.help_liststore.append([u"Información no disponible",""]) 

    def __expander_on_button_cb(self, widget, data):
        self.expander_on_button.hide()
        self.expander_off_button.show()
        self.main_toolbar.show_all()
        self.actions_scrolled_window.show_all()
        self.main_separator.show()
        self.mini_banner_eventbox.hide()
        self.expander_eventbox.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#ecf1fb"))
        self.MSDConf.set_ui_general_key_value("main_expander_on", True)
        self.MSDConf.save_conf()

    def __expander_off_button_cb(self, widget, data):
        self.expander_on_button.show()
        self.expander_off_button.hide()
        self.main_toolbar.hide_all()
        self.actions_scrolled_window.hide_all()
        self.main_separator.hide()
        self.mini_banner_eventbox.show()
        self.expander_eventbox.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#525774"))
        self.MSDConf.set_ui_general_key_value("main_expander_on", False)
        self.MSDConf.save_conf()

    def __consum_button_cb(self,data):
        self.consum.show_all()

    def __active_device_changed_cb(self, mcontroller, udi):
        self.__load_help_info()
        dev = self.mcontroller.get_active_device()
        if dev == None :
            self.device_selected_label.set_markup(u'<span foreground="white">Ningún dispositivo disponible</span>')
            self.card_statusbar.hide()
            return
        else:
            if MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
                self.card_statusbar.show()
            else:
                self.card_statusbar.hide()
                
        self.device_selected_label.set_markup ('<span foreground="white">%s</span>' % dev.get_property("pretty-name"))

    def __active_device_card_status_changed(self, mcontroller, status):
        context_id = self.main_statusbar.get_context_id("card_status")
        
        if status == MobileManager.CARD_STATUS_DETECTED :
            self.card_statusbar.pop(context_id,)
            self.card_statusbar.push(context_id, "Tarjeta detectada")
        elif status == MobileManager.CARD_STATUS_CONFIGURED :
            self.card_statusbar.pop(context_id,)
            self.card_statusbar.push(context_id, "Inicializando tarjeta")
        elif status == MobileManager.CARD_STATUS_NO_SIM or status == MobileManager.CARD_STATUS_ERROR :
            self.card_statusbar.pop(context_id,)
            self.card_statusbar.push(context_id, "Tarjeta no tiene SIM")
            msg = gtk.MessageDialog(parent=self.main_window, flags=0,
                                    type=gtk.MESSAGE_ERROR,
                                    buttons=gtk.BUTTONS_CLOSE, message_format=None)
            msg.set_markup("<b>SIM no encontrada</b>")
            msg.format_secondary_markup(u"La Tarjeta o Módem USB Internet Móvil no contiene una tarjeta SIM válida. "
                                        u"Por favor, comprueba que hay una tarjeta SIM introducida correctamente en "
                                        u"la Tarjeta o Módem USB Internet Móvil.")
            ret = msg.run()
            msg.destroy()
            dev = self.mcontroller.get_active_device()
            dev.turn_off()
        elif status == MobileManager.CARD_STATUS_PIN_REQUIRED :
            self.card_statusbar.pop(context_id,)
            self.card_statusbar.push(context_id, u"Comprobando código PIN")
            ask_pin = MobileManager.ui.MobileAskPinDialog(self.mcontroller)
            ask_pin.run()
        elif status == MobileManager.CARD_STATUS_PUK_REQUIRED :
            self.card_statusbar.pop(context_id,)
            self.card_statusbar.push(context_id, u"Comprobando código PIN")
            ask_puk = MobileManager.ui.MobilePukDialog(self.mcontroller)
            ask_puk.run()
        elif status == MobileManager.CARD_STATUS_OFF :
            self.card_statusbar.pop(context_id,)
            self.card_statusbar.push(context_id, "Tarjeta apagada")
        elif status == MobileManager.CARD_STATUS_ATTACHING :
            self.card_statusbar.pop(context_id,)
            self.card_statusbar.push(context_id, "Buscando red y registrando")
        elif status == MobileManager.CARD_STATUS_READY :
            self.card_statusbar.pop(context_id,)
            self.card_statusbar.push(context_id, "Tarjeta preparada")

    def __device_added_cb (self, mcontroller, udi):
        if len(self.mcontroller.get_available_devices()) == 1:
            devices_list = self.mcontroller.get_available_devices()
            added_device = devices_list[0]
            self.mcontroller.set_active_device(added_device.dev_props["info.udi"])
            return

        added_device = None
        for dev in self.mcontroller.get_available_devices() :
            if dev.dev_props["info.udi"] == udi :
                added_device = dev
                break
        active_device = self.mcontroller.get_active_device()
        
        added_device_priority = int(added_device.get_property("priority"))
        active_device_priority = int(active_device.get_property("priority"))
    
        if self.MSDConnManager.ppp_manager.status() != MobileManager.PPP_STATUS_CONNECTED:

            if added_device_priority > active_device_priority :
                self.mcontroller.set_active_device(added_device.dev_props["info.udi"])
            elif added_device_priority == active_device_priority :
                msg = gtk.MessageDialog(parent=self.main_window, flags=0,
                                        type=gtk.MESSAGE_INFO,
                                        buttons=gtk.BUTTONS_YES_NO, message_format=None)
                msg.set_markup("<b>Dispositivo detectado</b>")
                msg.format_secondary_markup(u"Se ha detectado la inserción del dispositivo :\n\n<b>%s</b>.\n\n"
                                            u"¿Deseas seleccionarlo como dispositivo GPRS/3G?"
                                            % added_device.get_property("pretty-name"))
                ret = msg.run()

                if ret == gtk.RESPONSE_YES :
                    self.mcontroller.set_active_device(added_device.dev_props["info.udi"])
                    set_active_dev_by_default(self.mcontroller)
                else:
                    self.mcontroller.set_active_device(active_device.dev_props["info.udi"])
                    if MobileManager.AT_COMM_CAPABILITY in added_device.capabilities :
                        added_device.turn_off()

                msg.destroy()
            else:
                self.mcontroller.set_active_device(active_device.dev_props["info.udi"])
                if MobileManager.AT_COMM_CAPABILITY in added_device.capabilities :
                    added_device.turn_off()
        else:
            if added_device_priority >= active_device_priority :
                msg = gtk.MessageDialog(parent=self.main_window, flags=0,
                                        type=gtk.MESSAGE_INFO,
                                        buttons=gtk.BUTTONS_YES_NO, message_format=None)
                msg.set_markup("<b>Dispositivo Detectado</b>")
                msg.format_secondary_markup(u"Se ha detectado la inserción del dispositivo :\n\n<b>%s</b>.\n\n"
                                            u"¿Deseas seleccionarlo como dispositivo GPRS/3G?"
                                            % added_device.get_property("pretty-name"))
                ret = msg.run()

                if ret == gtk.RESPONSE_YES :
                    active_device = self.mcontroller.get_active_device()
                    self.mcontroller.dialer.stop()
                    if MobileManager.AT_COMM_CAPABILITY in active_device.capabilities :
                        active_device.turn_off()
                    self.mcontroller.set_active_device(added_device.dev_props["info.udi"])
                    set_active_dev_by_default(self.mcontroller)
                else:
                    self.mcontroller.set_active_device(active_device.dev_props["info.udi"])
                    if MobileManager.AT_COMM_CAPABILITY in added_device.capabilities :
                        added_device.turn_off()

                msg.destroy()
            else:
                self.mcontroller.set_active_device(active_device.dev_props["info.udi"])
                if MobileManager.AT_COMM_CAPABILITY in added_device.capabilities :
                    added_device.turn_off()

            

    def __device_removed_cb(self, mcontroller, udi):
        if len(self.mcontroller.get_available_devices()) > 0 :
            if self.mcontroller.get_active_device() == None :
                devices_list = self.mcontroller.get_available_devices()
                new_active_device = devices_list[0]
                self.mcontroller.set_active_device(new_active_device.dev_props["info.udi"])
            else:
                dev = self.mcontroller.get_active_device()
                self.mcontroller.set_active_device(dev.dev_props["info.udi"])
        else:
            self.mcontroller.set_active_device(None)

    def __connected_cb(self, dialer):
        context_id = self.main_statusbar.get_context_id("contexto de conexiones")
        self.main_statusbar.pop(context_id,)
        self.main_statusbar.push(context_id, "Conectado")
        self.connect_statusbar_im.show()
        self.disconnect_statusbar_im.hide()
        
    def __connecting_cb(self, dialer):
        context_id = self.main_statusbar.get_context_id("contexto de conexiones")
        self.main_statusbar.pop(context_id,)
        self.main_statusbar.push(context_id, "Conectando")
        
    def __disconnected_cb(self, dialer):
        context_id = self.main_statusbar.get_context_id("contexto de conexiones")
        self.main_statusbar.pop(context_id,)
        self.main_statusbar.push(context_id, "Desconectado")
        self.connect_statusbar_im.hide()
        self.disconnect_statusbar_im.show()
        
    def __disconnecting_cb(self, dialer):
        context_id = self.main_statusbar.get_context_id("contexto de conexiones")
        self.main_statusbar.pop(context_id,)
        self.main_statusbar.push(context_id, "Desconectando")

    def __device_selected_button_cb(self,widget, data):
        print "device_selected_button_cb"
        dev = self.mcontroller.get_active_device()
        if dev == None :
            self.preferences.show_all(tab=3)
            return
        if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
            self.preferences.show_all(tab=3)
            return
        
        self.at_options_button.popup(None)

    def __initial_actions_cb(self):
        self.warning_dialog.run()   
        return False

    def __import_while_app_working_cb(self):
        list_dir = os.listdir(MSD.import_dir)
        if len(list_dir) > 0 :
            for f in list_dir:
                self.preferences.importer.import_from_file(os.path.join(MSD.import_dir, f))
                os.remove (os.path.join(MSD.import_dir, f))
        return True

    def __main_treview_inspector_cb(self):
        model = self.main_actions_view.get_model()
        tmp_iter = model.get_iter(0)

        while tmp_iter != None:
            codename = model.get_value(tmp_iter, 2)
            installed_value = self.MSDConf.get_action_key_value(codename, "installed")
            if installed_value == True:
                model.set_value(tmp_iter, 3, None)
            else:
                model.set_value(tmp_iter, 3, "#DEDEDE")
            tmp_iter = model.iter_next(tmp_iter)

        return True

