#!/usr/bin/python
# -*- coding: utf-8 -*
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica Móviles España S.A.U.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import MSD

class MSDBoem:
    def __init__(self):
        self.userid = "prueba"
        self.versionid = "5.0"
        self.serviceid = "12"
        self.apn = "kjskdjksjd"
        self.modem = "Modem"
        self.osid = "12"
        self.intraf = "1212"
        self.outtraf = "1212"
        self.initial_timestamp = "2323232"
        self.final_timestamp = "87238723"
        self.applicationid = "2323"
        self.connection = "skjdksjd"
        self.comutation = "skdjksjd"

        
    def close(self):
        reg =  [self.userid,
                self.versionid,
                self.serviceid,
                self.apn,
                self.modem,
                self.osid,
                self.intraf,
                self.outtraf,
                self.initial_timestamp,
                self.final_timestamp,
                self.applicationid,
                self.connection,
                self.comutation] 

        print reg
