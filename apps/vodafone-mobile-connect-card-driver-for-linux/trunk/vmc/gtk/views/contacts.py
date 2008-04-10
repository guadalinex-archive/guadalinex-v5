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
"""Views for contacts"""
__version__ = "$Rev: 1172 $"

import os.path

import gtk

from vmc.gtk import View
import vmc.common.consts as consts

class ContactView(View):
    """Base view for contacts"""
    GLADE_FILE = os.path.join(consts.GLADE_DIR, "contacts.glade")
    
    def __init__(self, *args, **kwds):
        View.__init__(self, *args, **kwds)
    
    def run(self):
        widget = self.get_top_widget()
        res = widget.run()
        widget.destroy()
        return res


class AddContactView(View):
    """View for add contact dialog"""
    GLADE_FILE = os.path.join(consts.GLADE_DIR, "contacts.glade")
    
    def __init__(self, ctrl):
        View.__init__(self, ctrl, self.GLADE_FILE,
                      'add_contact_window', register=False, domain="VMC")
        self.setup_view(ctrl)
        ctrl.register_view(self)
    
    def setup_view(self, ctrl):
        self.get_top_widget().set_position(gtk.WIN_POS_CENTER_ON_PARENT)
        self['alignment5'].add(ctrl.name_entry)
        self['alignment6'].add(ctrl.number_entry)
        self['alignment7'].add(ctrl.group_entry)

class SyncContactView(View):
    """View for sync contact dialog"""
    GLADE_FILE = os.path.join(consts.GLADE_DIR, "contacts.glade")
    
    def __init__(self, ctrl):
        View.__init__(self, ctrl, self.GLADE_FILE,
                      'sync_dialog', register=False, domain="VMC")
        self.setup_view(ctrl)
        ctrl.register_view(self)
    
    def setup_view(self, ctrl):
        self.get_top_widget().set_position(gtk.WIN_POS_CENTER_ON_PARENT)

class SearchContactView(ContactView):
    """View for the search contact dialog"""
    
    def __init__(self, ctrl):
        ContactView.__init__(self, ctrl, self.GLADE_FILE,
                      'search_dialog', register=True, domain="VMC")


class ContactsListView(ContactView):
    """View for the list of contacts in the send sms window"""
    
    def __init__(self, ctrl):
        ContactView.__init__(self, ctrl, self.GLADE_FILE,
                      'contacts_list_dialog', domain="VMC")
    
