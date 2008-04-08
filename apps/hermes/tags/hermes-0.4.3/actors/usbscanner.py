#!/usr/bin/python
# -*- coding: utf-8 -*-


#Módulo scaner- Módulo que implementa el "actor hardware" para los dispositivos
#escáner.
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
from utils.grepmap import UsbGrepMap

from usbdevice import Actor

from gettext import gettext as _

SCANNERICON = os.path.abspath('actors/img/scanner.png')
SCANNERICONOFF = os.path.abspath('actors/img/scanneroff.png')

# Set True for activate usb_device hack everytime. False for activate hack when
# a scanner is connected.

DEBUG = False


class UsbScannerActorHack(object):
    """
    This class is a hack for usb_device.Actor
    """
    PACKAGES = PkgDeviceActor.get_packages('usbscanner')

    def __init__(self):
        # Hacking usb.Actor class
        Actor.on_added = self.decor(Actor.on_added, self.hack_on_added)
        Actor.on_removed = self.decor(Actor.on_removed, self.hack_on_removed)


    def is_scanner(self, usb_actor):
        grepmap = UsbGrepMap()
        vid = usb_actor.properties['usb_device.vendor_id']
        pid = usb_actor.properties['usb_device.product_id']
        return grepmap.get_module(vid, pid) == 'libusbscanner'
        

    def decor(self, oldf, hack_function):
        "Python Decorator for on_added method."

        def new_method(usb_actor, *args, **kargs):
            oldf(usb_actor, *args, **kargs)
            if DEBUG or self.is_scanner(usb_actor):
                hack_function(usb_actor)

        return new_method


    def hack_on_added(self, usb_actor):
        "usbdevice.Actor.on_added hack"
        assert(isinstance(usb_actor, Actor))

        def run_xsane():
            os.system('xsane & ')

        def install_packages():
            synaptic.install(UsbScannerActorHack.PACKAGES)
            run_xsane()

        synaptic = PkgInstaller()

        actions = {}
        if synaptic.check(UsbScannerActorHack.PACKAGES):
            actions = {_("Run rendering program"): run_xsane}
        else:
            actions = {_("Install required packages"): install_packages}

        usb_actor.msg_render.show(_("SCANNER"), _("Scanner device detected"),
                                SCANNERICON, actions = actions)


    def hack_on_removed(self, usb_actor):
        "usbdevice.Actor.on_removed hack"
        assert(isinstance(usb_actor, Actor))

        usb_actor.msg_render.show(_("SCANNER"), _("Scanner device disconnected"),
                                SCANNERICONOFF)


ua = UsbScannerActorHack()

