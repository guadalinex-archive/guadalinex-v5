#!/usr/bin/python
# -*- coding: UTF-8 -*-
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

import os
import sys
import gtk
import gtk.glade

import gettext
import locale
import __builtin__

import emsync
import emtraffic

GETTEXT_DOMAIN = 'escritorio-movistar'
#locale.setlocale(locale.LC_ALL, '')
locale_dir = os.path.join("/usr/local", "share/" ,"locale/")
for module in gtk.glade, gettext:
    module.textdomain(GETTEXT_DOMAIN)
    module.bindtextdomain(GETTEXT_DOMAIN, locale_dir)

#para que funcione en desrrollo si tener que usar enlaces simbolicos 
if not os.path.exists("PPPManager"):
	sys.path.append(os.path.join(os.path.dirname(sys.argv[0]),"../backend"))

import MSD     

sys.path.append(MSD.msd_eggs_dir)

def main():
	# register the gettext function for the whole interpreter as "_"
	__builtin__._ = gettext.gettext
	
	#creo el lock
	f = None
	try:
		f = open(MSD.lock_file,"w")
		f.write("%s" % os.getpid())
	finally:
		if f is not None:
			f.close()
			
        x = MSD.MSDMainWindow()
        gtk.main()

if __name__ == '__main__':
    emsync.global_init()
    emtraffic.global_init()
    main()
    emtraffic.global_end()
    emsync.global_end()

