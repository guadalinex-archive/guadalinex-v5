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
"""SIM startup module"""

__version__ = "$Rev: 1172 $"

from twisted.python import log
from twisted.internet import defer

import vmc.common.exceptions as ex

class SIMBaseClass(object):
    """
    I take care of initing the SIM
    
    The actual details of initing the SIM vary from mobile to datacard, so
    I am the one to subclass in case your device needs a special startup
    """
    def __init__(self, sconn):
        super(SIMBaseClass, self).__init__()
        self.sconn = sconn
        self.size = None
        self.charset = 'IRA'
    
    def set_size(self, size):
        self.size = size
    
    def set_charset(self, charset):
        self.charset = charset
    
    def preinit(self):
        """
        What I do::
          - Reset settings
          - Disable echo
        """
        self.sconn.reset_settings().addCallbacks(lambda ignored: ignored,
                                        lambda failure: log.err(failure))
        
        def disable_echo_eb(failure):
            failure.trap(ex.CMEErrorOperationNotAllowed)
            log.err(failure)
        
        d = self.sconn.disable_echo()
        d.addCallback(lambda ignored: ignored)
        d.addErrback(disable_echo_eb)
        return d
    
    def _set_charset(self, charset):
        """
        Checks whether is necessary the change and memorizes the used charset
        """
        def process_charset(reply):
            """
            Only set the new charset if is different from current encoding
            """
            if reply != charset:
                return self.sconn.set_charset(charset)
                
            # we already have the wanted UCS2
            self.charset = reply
            return defer.succeed(True)
        
        d = self.sconn.get_charset()
        d.addCallback(process_charset)
    
    def _process_charsets(self, charsets):
        if 'UCS2' in charsets:
            self._set_charset('UCS2')
        elif 'IRA' in charsets:
            self._set_charset('IRA')
        elif 'GSM' in charsets:
            self._set_charset('GSM')
        else:
            msg = "Couldn't find an appropriated charset in %s"
            raise ex.CharsetError(msg % charsets)
    
    def _setup_encoding(self):
        d = self.sconn.get_available_charset()
        d.addCallback(self._process_charsets)
        return d
    
    def postinit(self, set_encoding=True):
        """
        Returns a C{Deferred} that will be callbacked when the SIM is ready
        
        What I do::
          - Set encoding to Unicode (if is possible, otherwise IRA or GSM
          - Whenever a new SMS is received I will save a copy in the SIM and
          will send a solicited SMS notification 
          - Set SMS format to PDU
        
        @param ucs2: If True, it will set the SIM's enconding to UCS2
        @type  ucs2: bool
        @rtype:      int
        @return:     The SIM's phonebook size
        """
        if set_encoding:
            self._setup_encoding()
        
        # Notification when a SMS arrives...
        self.sconn.set_sms_indication(2, 1)
        # set PDU mode
        self.sconn.set_sms_format(0)
        
        d = self.sconn.get_phonebook_size()
        def phonebook_size_cb(resp):
            # if we are in test, this is a regular expression
            if isinstance(resp, int):
                self.size = resp
            elif resp[0] and hasattr(resp[0], 'group'):
                self.size = int(resp[0].group('size'))
            
            return self.size
        
        d.addCallback(phonebook_size_cb)
        return d
        
