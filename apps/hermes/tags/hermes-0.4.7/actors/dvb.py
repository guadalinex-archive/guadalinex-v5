# -*- coding: utf-8 -*-


#Módulo dvb - Módulo que implementa el "actor hardware" para los
#dispositivos TDT 
#
#Copyright (C) 2007 Junta de Andalucía
#
#Autor/es (Author/s):
#
#- Gumersindo Coronel Pérez <gcoronel@emergya.info>
#- J. Félix Ontañón <fontanon@emergya.es>
#
#Este fichero es parte de Detección de Hardware de Guadalinex 
#
#Detección de Hardware de Guadalinex es software libre. Puede redistribuirlo y/o modificarlo 
#bajo los términos de la Licencia Pública General de GNU según es 
#publicada por la Free Software Foundation, bien de la versión 2 de dicha
#Licencia o bien (según su elección) de cualquier versión posterior. 
#
#Detección de Hardware de Guadalinex se distribuye con la esperanza de que sea útil, 
#pero SIN NINGUNA GARANTÍA, incluso sin la garantía MERCANTIL 
#implícita o sin garantizar la CONVENIENCIA PARA UN PROPÓSITO 
#PARTICULAR. Véase la Licencia Pública General de GNU para más detalles. 
#
#Debería haber recibido una copia de la Licencia Pública General 
#junto con Detección de Hardware de Guadalinex . Si no ha sido así, escriba a la Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
#
#-------------------------------------------------------------------------
#
#This file is part of Detección de Hardware de Guadalinex .
#
#Detección de Hardware de Guadalinex is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#at your option) any later version.
#
#Detección de Hardware de Guadalinex is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Foobar; if not, write to the Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import os.path

from utils.pkginstaller import PkgInstaller
from deviceactor import PkgDeviceActor
from gettext import gettext as _


def is_valid(value):
    valid_list = ['cinergyT', 'dvb-usb-a800', 'dvb-usb-dibusb-mb', 
            'dvb-usb-dtt200u', 'dvb-usb-vp702x', 'dvb-usb-vp7045',
            'dvb-usb-umt-010', 'b2c2', 'bt8xx', 'ttusb-budget'] 
    if (value in valid_list) or (value.startswith('dvb_usb')):
        return True


class Actor(PkgDeviceActor):

    __required__ = {'info.bus': 'usb',
            'info.linux.driver': is_valid,
            }

    # Important for compatibility with dvbfw !!
    __priority__ = 4

    __icon_path__  = os.path.abspath('actors/img/dvb.png')
    __iconoff_path__ = os.path.abspath('actors/img/dvboff.png')

    __device_title__ = 'DVB'
    __device_conn_description__ = _('Digital television device connected')
    __device_disconn_description__ = _('Digital television device disconnected')
    __device_use_title__ = _('Run TV-Player')
