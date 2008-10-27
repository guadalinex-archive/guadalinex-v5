# -*- coding: utf-8 -*-


#Módulo usbjoystick - Módulo que implementa el "actor hardware" para los
#dispositivos USB Joystick/Gamepad
#
#Copyright (C) 2008 Junta de Andalucía
#
#Autor/es (Author/s):
#
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

def disable_mouse_actor():
    from actors.usbmouse import Actor as MouseActor
    MouseActor.__enabled__ = False

def is_valid(value):
    valid_list = ['usb joystick', 'usb  joystick', 'unknown (0x1009)'] 
    if (value.lower() in valid_list):
	disable_mouse_actor()
        return True

class Actor(PkgDeviceActor):

    __required__ = {'info.bus':'usb_device',
	    'info.product': is_valid,
            }

    __icon_path__  = os.path.abspath('actors/img/joystick.png')
    __iconoff_path__ = os.path.abspath('actors/img/joystickoff.png')

    __device_title__ = 'Joystick'
    __device_conn_description__ = _('Joystick/Gamepad connected')
    __device_disconn_description__ = _('Joystick/Gamepad disconnected')
    __device_use_title__ = _('Calibrate joystick')
