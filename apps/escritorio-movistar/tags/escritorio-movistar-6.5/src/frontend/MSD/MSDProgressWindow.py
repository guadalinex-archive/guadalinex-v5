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
import os
import MSD
import gobject

class MSDProgressWindow:

    def __init__(self,cancel_callback=None):
        self.ui_xml =gtk.glade.XML(os.path.join(MSD.glade_files_dir , "progress_dialog.glade"))
        self.progress_dialog = self.ui_xml.get_widget("progress_dialog")
        self.progress_dialog.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        self.progressbar = self.ui_xml.get_widget("progressbar")

        self.image = self.ui_xml.get_widget("dialog_image")
        self.image.set_from_file(MSD.icons_files_dir + "movistar.gif")
        self.title_label=  self.ui_xml.get_widget("message_label")
        self.cancel_button = self.ui_xml.get_widget("cancel_button")
        self.cancel_button.connect("clicked",self.__cancel_button_cb)
        self.progress_dialog.connect("delete_event",self.__close_window_cb)
        self.timer_id = 0
        self.cancel_callback = cancel_callback
        self.show_buttons = True
        self.is_show = False
        
    def show(self,title=None,message=None,cancel_callback=None):
        self.is_show = True
        if title:
            self.progress_dialog.set_title(title)
        if message:
            self.title_label.set_text(message)
        if cancel_callback:
            self.cancel_callback = cancel_callback
            
        self.timer_id = gobject.timeout_add(100,self.__timer_cb)
            
        self.progress_dialog.show_all()
        self.progressbar.hide()
        if self.show_buttons == True:
            self.cancel_button.show()
        else:
            self.cancel_button.hide()
                
    def hide(self):
        self.is_show = False
        self.progress_dialog.hide()
        

    def set_show_buttons(self, show):
        self.show_buttons = show

    def set_image(self, stock_id):
        self.image.set_from_stock(stock_id, gtk.ICON_SIZE_DIALOG)

    def __timer_cb(self):
        if not self.progress_dialog.is_active():
            self.timer_id = None
            return False
        self.progressbar.pulse()
        return True
        
    def __cancel_button_cb(self, widget):
        self.hide()
        if self.cancel_callback :
            self.cancel_callback()

    def __close_window_cb(self,widget,event):
        self.__cancel_button_cb(widget)
        return True
