# -*- coding: utf-8 -*-
# Copyright (C) 2006-2007  Vodafone España, S.A.
# Author:  Pablo Martí
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
"""View for the new sms window"""
__version__ = "$Rev: 1172 $"

import os.path

import gtk

from vmc.gtk import View
import vmc.common.consts as consts
from vmc.common.encoding import _

HEIGHT = 200
WIDTH = 325

class NewSmsView(View):
    """View for the new sms window"""
    
    GLADE_FILE = os.path.join(consts.GLADE_DIR, "sms.glade")
    
    def __init__(self, ctrl):
        super(NewSmsView, self).__init__(ctrl, self.GLADE_FILE,
            'sms_edit_window', register=False, domain="VMC")
        self.setup_view(ctrl)
        ctrl.register_view(self)
    
    def setup_view(self, ctrl):
        self.get_top_widget().set_position(gtk.WIN_POS_CENTER_ON_PARENT)
        self.get_top_widget().set_size_request(width=WIDTH, height=HEIGHT)
        self['sms_edit_table'].attach(ctrl.numbers_entry, 1, 2, 0, 1)
        self.setup_custom_buttons(ctrl)
    
    def setup_custom_buttons(self, ctrl):
        items = [('vmc-send', '_Send', 0, 0, None)]
        aliases = [('vmc-send', gtk.STOCK_OK)]
        gtk.stock_add(items)
        factory = gtk.IconFactory()
        factory.add_default()
        style = self.get_top_widget().get_style()
        for new_stock, alias in aliases:
            icon_set = style.lookup_icon_set(alias)
            factory.add(new_stock, icon_set)
        
        bbox = self['sms_edit_hbutton_box']
        button = gtk.Button(stock='vmc-send')
        button.connect('clicked', ctrl.on_send_button_clicked)
        self['send_button'] = button
        bbox.pack_end(button)
    
    def set_busy_view(self):
        self['send_button'].set_sensitive(False)
        self['save_button'].set_sensitive(False)
    
    def set_idle_view(self):
        self['send_button'].set_sensitive(True)
        self['save_button'].set_sensitive(True)
    
class ForwardSmsView(NewSmsView):
    """View for forwarding SMS"""
    def __init__(self, ctrl):
        super(ForwardSmsView, self).__init__(ctrl)
    