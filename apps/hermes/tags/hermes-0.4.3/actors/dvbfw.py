#!/usr/bin/python
# -*- coding: utf-8 -*-

#Módulo dvbfw - Módulo que implementa el "actor hardware" para los
#dispositivos de TDT con problemas con el firmware 
#
#Copyright (C) 2007 Junta de Andalucía
#
#Autor/es (Author/s):
#
#- Gumersindo Coronel Pérez <gcoronel@emergya.info>
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

import os
import webbrowser

from gettext import gettext as _

from dvb import Actor as DvbActor

def get_usbmap():
    valid_list = [ 
            'cinergyT2', 'b2c2', 'bt8xx', 'ttusb-budget', 
            'pluto2', 'ttpci', 'ttusb-dec'
            ]

    kernel_version = os.popen('uname -r').read().strip()
    try:
        usbmap_file = open('/lib/modules/%s/modules.usbmap' % kernel_version)
    except IOError:
        return []

    result = []
    for line in usbmap_file.readlines():
        line = line.strip()
        if line.startswith('#'):
            continue
        raw_c = line.split()
        components = (raw_c[0], raw_c[2], raw_c[3])

        if (components[0] in valid_list) or\
                components[0].startswith('dvb-usb'):
            result.append(components)

    return result


def is_valid_vendor(value):
    i = 0
    value = int(value)
    global vendor_id
    vendor_id = None
    for vid in Actor.usbmap:
        if value == eval(vid[1]):
            vendor_id = value
            return True
        i += 1
    return False


def is_valid_product(value):
    if vendor_id:
        product_id = int(value)
        vplist = [(eval(ele[1]), eval(ele[2])) for ele in Actor.usbmap]
        return (vendor_id, product_id) in vplist



class Actor(DvbActor):
    """ DVB devices wich haven't got firmware files.
    """

    usbmap = get_usbmap()

    __required__ = {'info.bus': 'usb',
            'usb.vendor_id': is_valid_vendor,
            'usb.product_id': is_valid_product,
            }

    # Important for compatibility with dvb !!
    __priority__ = 3

    def on_added(self):

        def open_browser():
            webbrowser.open('http://www.guadalinex.org/distro/V4/hermes/tdt')

        self.msg_render.show_warning('TDT', 
                _('TDT device without firmware!!'),
                actions = {_('More info...'): open_browser})
