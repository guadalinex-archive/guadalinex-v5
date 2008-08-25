# -*- coding: utf-8 -*-

#Módulo usbkeyboard- Módulo que implementa el "actor hardware" para los
#teclados usb. 
#
#Copyright (C) 2008 Junta de Andalucía
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

import os.path

from deviceactor import DeviceActor
from gettext import gettext as _

KEYBICONON = os.path.abspath('actors/img/keyboard.png')
KEYBICONOFF = os.path.abspath('actors/img/keyboardoff.png')

def is_valid(value):
	return 'input' in value and 'input.keyboard' in value

class Actor (DeviceActor):

    __required__ = {'linux.subsystem':'input', 
		    'info.capabilities':is_valid}

    def on_added(self):
        self.msg_render.show(_("Keyboard"), _("USB keyboard configured"),
                icon = KEYBICONON)

    def on_removed(self):
        self.msg_render.show(_("Keyboard"), _("USB keyboard disconnected"),
                icon = KEYBICONOFF)
