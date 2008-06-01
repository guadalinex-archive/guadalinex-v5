# -*- coding: utf-8 -*-

#Módulo omnikeycardreader- Módulo que implementa el "actor hardware" para los
#lectores de tarjetas Omnikey Cardman 3121
#
#Copyright (C) 2007 Junta de Andalucía
#
#Autor/es (Author/s):
#
#- J. Félix Ontañón <fontanon@emergya.es>
#
#Este fichero es parte de Detección de Hardware de Guadalinex 2005 
#
#Detección de Hardware de Guadalinex 2005  es software libre. Puede redistribuirlo y/o modificarlo 
#bajo los términos de la Licencia Pública General de GNU según es 
#publicada por la Free Software Foundation, bien de la versión 2 de dicha
#Licencia o bien (según su elección) de cualquier versión posterior. 
#
#Detección de Hardware de Guadalinex 2005  se distribuye con la esperanza de que sea útil, 
#pero SIN NINGUNA GARANTÍA, incluso sin la garantía MERCANTIL 
#implícita o sin garantizar la CONVENIENCIA PARA UN PROPÓSITO 
#PARTICULAR. Véase la Licencia Pública General de GNU para más detalles. 
#
#Debería haber recibido una copia de la Licencia Pública General 
#junto con Detección de Hardware de Guadalinex 2005 . Si no ha sido así, escriba a la Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
#
#-------------------------------------------------------------------------
#
#This file is part of Detección de Hardware de Guadalinex 2005 .
#
#Detección de Hardware de Guadalinex 2005  is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#at your option) any later version.
#
#Detección de Hardware de Guadalinex 2005  is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Foobar; if not, write to the Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import commands
import os
import os.path

from deviceactor import PkgDeviceActor
from utils.pkginstaller import PkgInstaller
from utils import get_sudo
from gettext import gettext as _

CERTMANAGER_CMD = '/usr/bin/certmanager.py'

class Actor(PkgDeviceActor):

    __required__ = {
            "info.bus": "usb_device",
            "usb_device.vendor_id":0x076b,
            "usb_device.product_id":0x3021
    }

    __icon_path__  = os.path.abspath('actors/img/ltc31.png')
    __iconoff_path__ = os.path.abspath('actors/img/ltc31off.png')
    __device_title__ = _("OmniKey Cardman 3121")
    __device_conn_description__ = _("Card reader detected")
    __device_disconn_description__ = _("Card reader disconnected")

    def on_added(self):
        actions = {}
	def configure_dnie():
            os.system('%s --install-dnie' % CERTMANAGER_CMD)

        def configure_ceres():
            os.system('%s --install-ceres' % CERTMANAGER_CMD)

        if os.path.exists(CERTMANAGER_CMD):
            actions[_("Configure DNIe")] = configure_dnie
            actions[_("Configure FNMT-Ceres card")] = configure_ceres

        def add_user_to_scard():
            import pwd
	    # The os.getlogin() raises OSError: [Errno 25]
            # Moved to getpwuid
	    user = pwd.getpwuid(os.geteuid())[0]
            # get root access
            if get_sudo():
                cmd = '/usr/bin/gksudo /usr/sbin/adduser %s scard' % user
                status, output = commands.getstatusoutput(cmd)
                self.msg_render.show_info(_('Session restart needed'),
                                          _('You must restart your session to apply the changes'))

        status, output = commands.getstatusoutput('/usr/bin/groups')
        if 'scard' not in output:
            actions[_("Add user to scard group")] = add_user_to_scard

        self.msg_render.show(self.__device_title__, 
                             self.__device_conn_description__,
                             self.__icon_path__, actions=actions)



