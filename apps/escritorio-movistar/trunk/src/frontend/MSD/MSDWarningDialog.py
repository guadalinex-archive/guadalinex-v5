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

class MSDWarningDialog:
    def __init__(self, xml, parent, conf):
        self.xml = xml
        self.conf = conf
        self.dialog = self.xml.get_widget ("warning_dlg")
        self.dialog.set_title("Escritorio movistar - Aviso")
	self.ok_button = self.xml.get_widget ("ok_btn")
        self.wellcome_warning_checkbutton = self.xml.get_widget("wellcome_warning_checkbutton")
        
	self.dialog.connect("delete_event", self.__closedialog_delete_event_cb)
	self.ok_button.connect("clicked", self.__closedialog_cb, None)
        self.wellcome_warning_checkbutton.connect("toggled", self.__wellcome_warning_toggled_cb, None)

        self.dialog.set_transient_for (parent)

    def __wellcome_warning_toggled_cb (self, widget, data):
        self.conf.set_ui_general_key_value("wellcome_warning_show", not widget.get_active())
        self.conf.save_conf()
    
    def __closedialog_cb (self, widget, data):
	self.dialog.hide ()

    def __closedialog_delete_event_cb (self, widget, data):
        self.__closedialog_cb(widget, data)
        return False
        
    def run(self):
        if self.conf.get_ui_general_key_value("wellcome_warning_show") == True:
            self.ok_button.grab_focus()
            self.dialog.run()
        
