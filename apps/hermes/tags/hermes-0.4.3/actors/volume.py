# -*- coding: utf-8 -*-

#Módulo volumeactor- Módulo que implementa el "actor hardware" para los
#dispositivos de volumen (dispositivos que se montan como unidades de disco) 
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

import glob
import os.path

from deviceactor import DeviceActor
from gettext import gettext as _

VOLUMEICON = os.path.abspath('actors/img/volume.png') 

class Actor (DeviceActor):

    __required__ = {'info.category': 'volume'}
    __listener_factories__ = []

    def __init__(self, *args, **kwargs):
        super(Actor, self).__init__(*args, **kwargs)

        self.listeners = [factory() for factory in self.__listener_factories__]

    def register_listener(cls, listener):
        cls.__listener_factories__.append(listener)

    register_listener = classmethod(register_listener)

    #def on_added(self):
    #    self.msg_render.show_info("Dispositivo de volumen conectado")

    def on_modified(self, key):
        if key == 'volume.is_mounted':
            try:
                if self.properties['volume.is_mounted']:
                    mount_point = self.properties['volume.mount_point']

                    def open_volume():
                        os.system('nautilus "%s"' % mount_point) 

                    self.message_render.show(_("Storage"), 
                        _("Device mounted on"), VOLUMEICON,
                        actions = {mount_point: open_volume})

                    for listener in self.listeners:
                        if listener.is_valid(self.properties):
                            listener.volume_mounted(mount_point)

                else:
                    self.message_render.show(_("Storage"),
                            _("Device unmounted"), VOLUMEICON) 

                    for listener in self.listeners:
                        if listener.is_valid(self.properties):
                            listener.volume_unmounted()

            except Exception, e:
                self.logger.error(_("Error:") + " " + str(e))

class AutoRegister(type):
    """This meta class auto register each class as an Actor listener"""
    def __new__(mcs, name, bases, dic):
        t = type.__new__(mcs, name, bases, dic)
        if name != 'VolumeListener': # don't register the abstract base class
            Actor.register_listener(t)
        return t

class VolumeListener(object):
    """A Volume Listener is something that wants notifications when the
    state of a volume changes"""

    __metaclass__ = AutoRegister

    def is_valid(self, properties):
        """This method acts like a filter.

        Based on the HAL properties this method should returns

          - True if this listener should be used

          - False otherwise
        """
        return False

    def volume_mounted(self, mount_point):
        """This is called when the volume is mounted"""

    def volume_unmounted(self):
        """This is called when the volume is unmounted"""

CERTMANAGER_CMD = '/usr/bin/certmanager.py'

class CertificateListener(VolumeListener):
    """Call cert_manager to detect certificates in the volume and to
    setup the main applications to use them

    Only valid for USB storage disks.
    """
    def __init__(self):
        super(CertificateListener, self).__init__()
        self.mount_point = None

    def is_valid(self, properties):
        if properties.get('volume.mount_point', None) == u'/media/usbdisk':
            if glob.glob('/media/usbdisk/*.p12'):
                return True
        return False

    def volume_mounted(self, mount_point):
        self.mount_point = mount_point

        if os.path.exists(CERTMANAGER_CMD):
            os.system('%s -p %s' % (CERTMANAGER_CMD, self.mount_point))

    def volume_unmounted(self):
        if os.path.exists(CERTMANAGER_CMD):
            user_dir = os.path.expanduser('~')
            log_file = os.path.join(user_dir, '.certmanager.log')
            if os.path.exists(log_file):
                os.system('%s -u %s' % (CERTMANAGER_CMD, log_file))
        self.mount_point = None
