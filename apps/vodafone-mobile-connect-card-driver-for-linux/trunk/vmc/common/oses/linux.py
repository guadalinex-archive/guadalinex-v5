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
Linux-based OS plugin
"""

__version__ = "$Rev: 1172 $"

import re

from twisted.python.procutils import which
from twisted.internet.utils import getProcessOutput

from vmc.common.dialers.wvdial import WvdialDialer
from vmc.common.oses.unix import UnixPlugin
from vmc.utils.utilities import LSB_INFO
from vmc.common.runtime import app_is_frozen

if app_is_frozen():
    lsb_info = None
else:
    lsb_info = LSB_INFO


class LinuxPlugin(UnixPlugin):
    """
    OSPlugin for Linux-based distros
    """
    
    dialer = WvdialDialer()
    
    def __init__(self):
        super(LinuxPlugin, self).__init__()
    
    def check_dialer_assumptions(self):
        return self.dialer.check_assumptions()
    
    def check_permissions(self):
        return self.dialer.check_permissions()
    
    def get_connection_args(self, dialer):
        assert dialer.binary == 'wvdial'
        
        if not self.privileges_needed:
            return [dialer.bin_path, self.abstraction['WVDIAL_CONN_SWITCH'],
                    dialer.conf_path, 'connect']
        else:
            gksudo_name = self.abstraction['gksudo_name']
            gksudo_path = which(gksudo_name)[0]
            return [gksudo_path, dialer.bin_path,
                    self.abstraction['WVDIAL_CONN_SWITCH'],
                    dialer.conf_path, 'connect']
    
    def get_disconnection_args(self, dialer):
        assert dialer.binary == 'wvdial'
        
        killall_path = which('killall')[0]
        if not self.privileges_needed:
            return [killall_path, 'pppd', 'wvdial']
        else:
            gksudo_name = self.abstraction['gksudo_name']
            gksudo_path = which(gksudo_name)[0]
            return [gksudo_path, killall_path, 'pppd', 'wvdial']
    
    def get_iface_stats(self, iface='ppp0'):
        CMD = 'cat'
        PATH = '/proc/net/dev'
        regexp = re.compile(r"""
            \s+%(iface)s:\s+
            (?P<in>\d+)
            \s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+
            (?P<out>\d+)
            """ % dict(iface=iface), re.VERBOSE)
        
        def _parse_input(text):
            """Extracts the recv and sent bytes from /proc/net/dev"""
            inbits = None
            outbits = None
            match = regexp.search(text)
            if match:
                # /proc/net/dev counts bytes not bits
                inbits = int(match.group('in')) * 8
                outbits = int(match.group('out')) * 8
            
            return [inbits, outbits]
        
        d = getProcessOutput(CMD, args=[PATH])
        d.addCallback(_parse_input)
        return d
    
    def is_valid(self, os_info=lsb_info):
        if not os_info:
            # lsb_info will be None on any OS != Linux
            return False
        
        try:
            if self.os_name.match(os_info['os_name']):
                if self.os_version:
                    if self.os_version.match(os_info['os_version']):
                        return True
                    else:
                        # version doesn't matchs
                        return False
                else:
                    # No version specified
                    return True
            else:
                # distro name doesn't matchs
                return False
        except KeyError:
            return False

