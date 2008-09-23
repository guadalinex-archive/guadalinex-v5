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
The hardware module manages device discovery via dbus/hal on Unix/Linux
"""
__version__ = "$Rev: 1172 $"

from twisted.internet.defer import deferredGenerator, waitForDeferred

from vmc.common.exceptions import (DeviceNotFoundError,
                                   DeviceLacksExtractInfo,
                                   UnknownPluginNameError)
from vmc.common.hardware._dbus import DbusComponent
from vmc.common.hardware.base import DeviceResolver
from vmc.common.interfaces import IDBusDevicePlugin
from vmc.common.plugin import PluginManager, UnknownDevicePlugin


class HardwareRegistry(DbusComponent):
    """Hardware Registry"""
    
    def __init__(self):
        super(HardwareRegistry, self).__init__()
        self.resolver = DeviceResolver()
    
    def get_plugin_from_name(self, name, dev):
        try:
            plugin = PluginManager.get_plugin_by_remote_name(name)
            plugin.dport = dev.dport
            plugin.baudrate = dev.baudrate
            if dev.cport:
                plugin.cport = dev.cport
            return plugin
        except UnknownPluginNameError, e:
            # we have a device that share's the ID with other plugin (potentially
            # based on the same chipset) but its remote name differs, this is
            # probably gonna be a new revision of a model, we will risk ourselves
            # and use the most similar plugin we know of
            return dev
    
    def get_plugin_for_remote_dev(self, speed, dport, cport):
        dev = UnknownDevicePlugin()
        from vmc.common.hardware.base import Customizer
        dev.custom = Customizer()
        dev.dport, dev.cport, dev.baudrate = dport, cport, speed
        d = DeviceResolver.identify_device(dev)
        d.addCallback(self.get_plugin_from_name, dev)
        return d
    
    def _get_dbus_devices(self):
        """
        Returns a list with all the IDBusDevicePlugin that might be present
        
        This list is potentially unaccurate as devices from the same company
        tend to share product_ids and you might get plugins that aren't
        present
        
        @raise DeviceLacksExtractInfo: If the device found lacks any of the
        necessary control/data ports.
        @raise DeviceNotFoundError: If no known device was found
        """
        exceptions = []
        found_plugins = []
        device_properties = self.get_devices_properties()
        
        for plugin in PluginManager.get_plugins(IDBusDevicePlugin):
            # DBusDevicePlugin implements __eq__ to be compared with dicts
            if plugin in device_properties:
                try:
                    plugin.setup(device_properties)
                    found_plugins.append(plugin)
                except DeviceLacksExtractInfo, e:
                    exceptions.append(e)
        
        if not found_plugins:
            if exceptions:
                # raise the first (and hopefully only) exception
                raise exceptions[0]
            else:
                # no devices and no exceptions, raise DeviceNotFoundError
                raise DeviceNotFoundError
        
        return found_plugins
    
    def get_devices(self):
        devices = []
        try:
            found_plugins = self._get_dbus_devices()
        except:
            raise
        
        # found plugins might be potentially full of plugins that share the
        # vendor and product id (like Huawei's E220/E270/E272), we'll have to
        # do some cleaning
        while found_plugins:
            plugin = found_plugins.pop(0)
            model = waitForDeferred(DeviceResolver.identify_device(plugin))
            yield model; model = model.getResult()
            
            foundplugin = self.get_plugin_from_name(model, plugin)
            if foundplugin and foundplugin not in devices:
                devices.append(foundplugin)
        
        yield devices; return
    
    get_devices = deferredGenerator(get_devices)
