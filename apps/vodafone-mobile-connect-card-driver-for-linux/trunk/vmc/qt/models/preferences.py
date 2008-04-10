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
"""
Preferences-related models
"""
__version__ = "$Rev: 1172 $"

from datetime import timedelta

import gobject

from vmc.gtk import ListStoreModel
from vmc.gtk.models.base import BaseWrapperModel

from vmc.utils.utilities import dict_reverter
from vmc.common.encoding import _

VALIDITY_DICT = {
     _('Maximum time').encode('utf8') : 'maximum',
     _('1 week').encode('utf8') : '1week',
     _('3 days').encode('utf8') : '3days',
     _('1 day').encode('utf8') : '1day',
}

VALIDITY_DICT_REV = dict_reverter(VALIDITY_DICT)

transform_validity = {
    'maximum' : timedelta(days=63),
    '1week' : timedelta(days=7),
    '3days' : timedelta(days=3),
    '1day' : timedelta(days=1),
}
    
class PreferencesModel(BaseWrapperModel):
    """Model for the preferences window"""
    
    def __init__(self, wrapper):
        super(PreferencesModel, self).__init__(wrapper)

class SMSCListStoreModel(ListStoreModel):
    """Store Model for smsc list combobox"""
    def __init__(self):
        super(SMSCListStoreModel, self).__init__(gobject.TYPE_PYOBJECT)
        self.active = None
    
    def add_smscs(self, smsc_list):
        return map(self.add_smsc, smsc_list)
    
    def add_smsc(self, smscobj):
        if smscobj.active:
            self.active = self.append([smscobj])
            return self.active
        
        return self.append([smscobj])


class SMSCItem(object):
    def __init__(self, message, number=None, active=True):
        self.message = message
        self.number = number
        self.active = active
    