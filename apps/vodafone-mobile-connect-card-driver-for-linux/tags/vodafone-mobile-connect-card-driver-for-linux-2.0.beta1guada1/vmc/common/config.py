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
Config related classes and methods

The object L{config} is a singleton instance of VMCConfig. The overhead of
opening the object, performing a get/set and closing it was too much.
That's why its a singleton. The instance is closed during
L{vmc.common.shutdown.shutdown_core()}. You must be careful as importing it
too early during the first run of the program, could cause that the config
file is not created yet and you'd get a messy traceback. Delay its import as
much as possible in modules used at startup or that are imported at startup.
"""
__version__ = "$Rev: 1172 $"

from vmc.common.consts import VMC_CFG
from vmc.common.configbase import VMCConfigBase, MobileProfile

class VMCConfig(VMCConfigBase):
    def __init__(self, path=VMC_CFG):
        super(VMCConfig, self).__init__(path)
        self.current_profile = None
        self._initialize_profile()
    
    def _initialize_profile(self):
        name = self.get('profile', 'name')
        # name might not be set if we've just started
        if name:
            profile = MobileProfile.from_name(name)
            self.set_current_profile(profile)
    
    def set_current_profile(self, profile):
        if self.current_profile:
            self.current_profile.write()
        
        self.set('profile', 'name', profile.name)
        self.current_profile = profile
        self.write()
    
    def set_last_device(self, cached_device_id):
        #XXX: Store last device in current profile
        self.set('profile', 'last_device', cached_device_id)
        self.write()

    def get_last_device(self):
        #XXX: Store last device in current_profile
        return self.get('profile', 'last_device')
    
    def close(self):
        super(VMCConfig, self).close()
        if self.current_profile:
            self.current_profile.close()
            self.current_profile = None


# configuration singleton
config = VMCConfig()
