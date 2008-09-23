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
DevicePlugin for the Sierra Wireless 875 datacard
"""

__version__ = "$Rev: 1172 $"

from vmc.common.exceptions import DeviceLacksExtractInfo
from vmc.common.plugin import DBusDevicePlugin

from vmc.common.hardware.sierra import SierraWirelessCustomizer

class SierraWireless875(DBusDevicePlugin):
    """L{vmc.common.plugin.DBusDevicePlugin} for SierraWireless 875"""
    name = "SierraWireless 875"
    version = "0.1"
    author = "anmsid & kgb0y"
    custom = SierraWirelessCustomizer
    
    __remote_name__ = "AC875"   #response from AT+CGMM

    __properties__ = {
        'usb_device.vendor_id' : [0x1199],
        'usb_device.product_id': [0x6820],
    }
    
    def extract_info(self, children):
        # HW 220 uses ttyUSB0 and ttyUSB1
        for device in children:
            try:
                if device['serial.port'] == 0: # data port
                    self.dport = device['serial.device'].encode('utf8')
            except KeyError:
                pass
        
        if not self.dport:
            raise DeviceLacksExtractInfo(self)

sierrawireless875 = SierraWireless875()
