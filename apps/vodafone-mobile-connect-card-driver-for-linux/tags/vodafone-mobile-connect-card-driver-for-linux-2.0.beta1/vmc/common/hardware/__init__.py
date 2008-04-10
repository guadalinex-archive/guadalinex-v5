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
The hardware module manages device discovery via dbus/hal
"""
__version__ = "$Rev: 1172 $"

import os

from vmc.common.encoding import _
from vmc.utils.utilities import dict_reverter

from vmc.common.hardware._dbus import DeviceListener, DbusComponent
if os.name == 'posix':
    from vmc.common.hardware._unixhardwarereg import HardwareRegistry
elif os.name == 'nt':
    from vmc.common.hardware._winhardwarereg import HardwareRegistry


CONN_OPTS_LIST = [
   unicode(_('GPRS only'), 'utf8'),
   unicode(_('3G only'), 'utf8'),
   unicode(_('GPRS preferred'), 'utf8'),
   unicode(_('3G preferred'), 'utf8'),
]

CONN_OPTS_DICT = {
   unicode(_('GPRS only'), 'utf8') : 'GPRSONLY',
   unicode(_('3G only'), 'utf8') : '3GONLY',
   unicode(_('GPRS preferred'), 'utf8') : 'GPRSPREF',
   unicode(_('3G preferred'), 'utf8') : '3GPREF'
}

CONN_OPTS_DICT_REV = dict_reverter(CONN_OPTS_DICT)

__all__ = ["CONN_OPTS_LIST", "CONN_OPTS_LIST_REV", "DbusComponent",
           "DeviceListener", "HardwareRegistry"]
