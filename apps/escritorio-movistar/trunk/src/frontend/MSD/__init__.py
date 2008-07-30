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

import os
import sys
import gtk
import gtk.glade
import gettext

GETTEXT_DOMAIN = 'escritorio-movistar'

locale_dir = os.path.join("/usr", "share/" ,"locale/")

def init_i18n():
    for module in gtk.glade, gettext:
        module.bindtextdomain(GETTEXT_DOMAIN, MSD.locale_dir)
        module.textdomain(GETTEXT_DOMAIN)

for module in gtk.glade, gettext:
    module.bindtextdomain(GETTEXT_DOMAIN, locale_dir)
    module.textdomain(GETTEXT_DOMAIN)

from MSDActionsManager import MSDActionsManager
from MSDAction import MSDAction
from MSDMainWindow import MSDMainWindow
from MSDMainToolbar import MSDMainToolbar
from MSDConsumWindow import MSDConsumWindow
from MSDConsumGraph import MSDConsumGraph
from MSDTrafficHistoryGraph import MSDTrafficHistoryGraph
from MSDMainStatsWidget import MSDMainStatsWidget
from MSDPreferencesDialog import MSDPreferencesDialog
from MSDAddressbookTab import MSDAddressbookTab
from MSDBookmarksTab import MSDBookmarksTab
from MSDServicesTab import MSDServicesTab
from MSDConnectionsTab import MSDConnectionsTab
from MSDSecurityTab import MSDSecurityTab
from MSDDevicesTab import MSDDevicesTab
from MSDGeneralTab import MSDGeneralTab
from MSDWarningDialog import MSDWarningDialog
from MSDProgressWindow import MSDProgressWindow
from MSDConf import MSDConf
from MSDExporter import MSDExporter
from MSDImporter import MSDImporter
from MSDRSSFeeder import MSDRSSFeeder

from MSDConsumManager import MSDConsumManager
from MSDMessages import *
from MSDConnectionsManager import MSDConnectionsManager
from MSDUtils import *
from MSDSecurityManager import MSDSecurityManager

from MSDSystray import MSDSystray

from MSDUpdateChecker import MSDUpdateChecker

from MobileManagerClient import *

EM_TYPE_GENERIC ="generic"
EM_TYPE_OPTION ="option"

base_dir = os.path.dirname(sys.argv[0])
share_files_dir= os.path.join("/usr","share/","escritorio-movistar/")
glade_files_dir= os.path.join(share_files_dir,"glade/")
icons_files_dir= os.path.join(share_files_dir,"icons/")
actions_data_dir= os.path.join("/usr","share/","escritorio-movistar/","Actions")
actions_py_dir= os.path.join("/usr/lib/python2.5/site-packages","MSD/","Actions")
msd_eggs_dir= os.path.join("/usr/lib/python2.5/site-packages","MSD/","eggs")
conf_file = os.path.join(os.environ["HOME"], ".movistar_desktop/conf")
default_device_file = os.path.join(os.environ["HOME"], ".movistar_desktop/default_device")
stats_file = os.path.join(os.environ["HOME"], ".movistar_desktop/stats.dat")
import_dir = os.path.join(os.environ["HOME"], ".movistar_desktop/import")
lock_file = os.path.join(os.environ["HOME"], ".movistar_desktop","escritoriom.lock")
startup_file = os.path.join(os.environ["HOME"], ".movistar_desktop","startup")
os.system("mkdir -p %s" % import_dir)

msd_version_major = 6
msd_version_minor = 5
msd_version_update = 0
msd_version_string = "6.5.0"

#retona el tipo de escriotrio
def get_md_type():
    if len(sys.argv) < 2 :
        return EM_TYPE_GENERIC
    if sys.argv[1] == EM_TYPE_OPTION:
        return EM_TYPE_OPTION
    #por defeto es generico
    return EM_TYPE_GENERIC
        

#comprueba si el MD es del tipo EM_TYPE_OPTION
def is_option_md_type():
    if get_md_type() == EM_TYPE_OPTION:
        return True
    else:
        return False
    
help_uri = os.path.join("/usr","share","doc","escritorio-movistar","help","index.htm")
help_online_uri = "http://www.atencionenlinea.movistar.es/"
copiagenda_uri = "http://www.copiagenda.movistar.es"

