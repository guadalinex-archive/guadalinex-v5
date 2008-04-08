#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys

import gtk
import gobject
from egg.trayicon import TrayIcon

class HermesTrayIcon(TrayIcon):

    def __init__(self):
        TrayIcon.__init__(self, 'Hermes TrayIcon')

        # Set up icon 
        self.button = gtk.Button()
        self.button.set_relief(gtk.RELIEF_NONE)
        event_box = gtk.EventBox()
        event_box.connect("button-press-event", self.on_mouse_press)
        image = gtk.Image()
        event_box.add(image)
        image.set_from_file('/usr/share/hermes/img/logo_16.png')
        self.add(event_box)

        # Set up menu
        self.menu = HermesMenu()

        self.show_all()


    def on_mouse_press(self, widget, event):
        if event.button == 3:
            self.menu.popup(None, None, None,
                    event.button, event.time)
            self.menu.show_all()

        elif event.button ==  1:
            self.menu.on_open()

        return True



class HermesMenu(gtk.Menu):

    def __init__(self):
        gtk.Menu.__init__(self)

        self._configure()
    

    def _configure(self):
        # Open coldassistant
        item = gtk.MenuItem('Abrir el asistente de acciones')
        item.connect('activate', self.on_open)
        self.append(item)

        # Separator
        self.append(gtk.SeparatorMenuItem())

        # Quit option
        quit_item = gtk.ImageMenuItem(stock_id = gtk.STOCK_QUIT)
        quit_item.connect('activate', self.on_quit)
        self.append(quit_item)


    def on_quit(self, widget):
        gtk.main_quit()
        sys.exit(0)


    def on_open(self, widget = None):
        cursor = gtk.gdk.Cursor(gtk.gdk.WATCH)
        self.get_screen().get_root_window().set_cursor(cursor)
        os.system('hcoldassistant &')
        gobject.timeout_add(2000, self.__timeout)


    def __timeout(self):
        cursor = gtk.gdk.Cursor(gtk.gdk.TOP_LEFT_ARROW)
        self.get_screen().get_root_window().set_cursor(cursor)
        # Exit loop
        return False

