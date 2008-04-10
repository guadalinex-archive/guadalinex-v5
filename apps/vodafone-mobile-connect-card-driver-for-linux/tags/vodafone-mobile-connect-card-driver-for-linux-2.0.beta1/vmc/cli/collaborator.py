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

__version__ = "$Rev: 1172 $"

from zope.interface import implements

from twisted.internet.threads import deferToThread
from twisted.internet.defer import (Deferred, deferredGenerator,
                                    waitForDeferred, succeed)

from vmc.common.interfaces import ICollaborator, ICollaboratorFactory

class CLICollaborator(object):
    """
    CLICollaborator implements the C{ICollaborator} interface on CLI
    """
    implements(ICollaborator)
    
    def __init__(self, device, config=None):
        self.keyring = None
        self.config = config
        self.device = device
        self.pin = None
        self.puk = None
        self.puk2 = None
    
    def get_pin(self):
        if self.config and self.config['pin']:
            return succeed(self.config['pin'])
        
        deferred = Deferred()
        
        return deferToThread(raw_input, "Insert your PIN:")
    
    def get_puk(self):
        pin = waitForDeferred(deferToThread(raw_input, "Insert your PIN:"))
        yield pin
        the_pin = pin.getResult()
        
        puk = waitForDeferred(deferToThread(raw_input, "Insert your PUK:"))
        yield puk
        the_puk = puk.getResult()
        
        yield (the_puk, the_pin); return
    
    get_puk = deferredGenerator(get_puk)
    
    def get_puk2(self):
        pin = waitForDeferred(deferToThread(raw_input, "Insert your PIN:"))
        yield pin
        the_pin = pin.getResult()
        
        puk2 = waitForDeferred(deferToThread(raw_input, "Insert your PUK2:"))
        yield puk2
        the_puk2 = puk2.getResult()
        
        yield (the_puk2, the_pin); return
    
    get_puk2 = deferredGenerator(get_puk2)


class CLICollaboratorFactory(object):
    implements(ICollaboratorFactory)
    
    @classmethod
    def get_collaborator(cls, device, *args, **kwds):
        return CLICollaborator(device, *args, **kwds)

