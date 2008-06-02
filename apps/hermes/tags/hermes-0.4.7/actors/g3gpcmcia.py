# -*- coding: utf-8 -*-


#Módulo bluetooth - Módulo que implementa el "actor hardware" para los
#dispositivos bluetooth
#
#Copyright (C) 2005 Junta de Andalucía
#
#Autor/es (Author/s):
#
#- Gumersindo Coronel Pérez <gcoronel@emergya.info>
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

from utils.pkginstaller import PkgInstaller
from deviceactor import PkgDeviceActor
from gettext import gettext as _
import logging

# [(vendor_id, product_id)]
# This VP_IDS is valid for g3gusb.Actor too.
VP_IDS = [
        (4817, 4097), 
        (6449, 12),
        (4817, 4099)
        ]

product_id = None
vendor_id = None

def is_valid_vendor(value):
    logging.getLogger().debug('g3g is_valid_vendor(%s)' % value)
    value = int(value)
    global vendor_id
    vendor_id = None
    if product_id:
        vendor_id = value
        return (vendor_id, product_id) in VP_IDS
    elif value in [ele[0] for ele in VP_IDS]:
            vendor_id = value
            return True
    return False


def is_valid_product(value):
    logging.getLogger().debug('g3g is_valid_product(%s)' % value)
    value = int(value)
    global product_id
    product_id = None
    if vendor_id:
        product_id = value
        return (vendor_id, product_id) in VP_IDS
    elif value in [ele[1] for ele in VP_IDS]:
        product_id = value
        return True
    return False



class Actor(PkgDeviceActor):

    __required__ = {
            'pci.vendor_id': is_valid_vendor,
            'pci.product_id': is_valid_product
            }

    __priority__ = 4

    __icon_path__  = os.path.abspath('actors/img/g3g.png')
    __iconoff_path__ = os.path.abspath('actors/img/g3goff.png')

    __device_title__ = '3G'
    __device_conn_description__ = _('3G device connected')
    __device_disconn_description__ = _('3G device disconnected')



