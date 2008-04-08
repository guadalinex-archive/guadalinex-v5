#!/usr/bin/python
# -*- coding: utf-8 -*-

#Módulo deviceactor - Módulo que contiene la clase DeviceActor, clase base para
#la creación de "actores de hardware"
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



"""
¿Cómo crear un Actor?

1) Creamos dentro del directorio actors un fichero .py, por ejemplo:
    actors/tvactor.py

2) Dentro de este fichero, lo primero que hacemos es importar la clase
DeviceActor dentro del módulo deviceactors:
        
   from deviceactor import DeviceActor

3) Creamos una clase que ha de llamarse Actor, y que hereda de DeviceActor:

    class Actor (DeviceActor):

4) Indicamos, a través del atributo de clase __required__ (de tipo dict), el nombre
y el valor de las propiedades requeridas para que el actor se active:

    __require__ = {'info.category' : 'tv'}


5) Redefinimos los métodos "on_added", "on_removed" y on_modified, con las acciones que se
deben realiza cuando se añada un dispositivo, se retire o se modifique una
propiedad del dispositivo para el que estamos actuando, respectivamente

Dentro de cualquier clase Actor disponemos de dos atributos sumamente útiles:

    self.msg_render: Es un objeto que implementa la interfaz
    org.guadalinex.TrayInterface, el cual usamos para comunicarnos con el
    usuario

    self.properties: Es un diccionario que contiene todas las propiedades del
    dispositivo sobre el que estamos actuando, proporcionado por HAL


El ejemplo completo:

------ actors/tvactor.py ------

from deviceactor import DeviceActor

class Actor(DeviceActor):
    __required__ ={'info.category': 'tv'}

    def on_added(self):
        self.msg_render.show_info("Dispositivo de tv detectado")

    def on_removed(self):
        self.msg_render.show_info("Dispositivo de tv desconectado")

    

"""

import os
import logging

import dbus
import syck

from utils.pkginstaller import PkgInstaller
from gettext import gettext as _


class DeviceActor(object):
    """
    Esta clase encapsula la lógica de actuación ante eventos en un dispositivo,

    Es una clase abstracta que sirve para crear "actores de dispositivos",
    que sirven para mostrar mensajes al insertar/quitar dispositivos
    """
    __required__ = {} 
    __priority__ = 3 # Values: 1, 2, 3, 4, 5. The higher value, the more priority.

    def __init__(self, message_render, device_properties):
        self.message_render = self.msg_render = message_render
        self.properties = device_properties
        self.msg_no = None
        self.udi = device_properties['info.udi']
        self.logger = logging.getLogger()


    def __on_property_modified(self, udi, num, values):
        for ele in values:
            key = ele[0]

            #Update properties
            obj = self.bus.get_object('org.freedesktop.Hal', self.udi)
            obj = dbus.Interface(obj, 'org.freedesktop.Hal.Device')
            self.properties = obj.GetAllProperties()

            self.on_modified(key)


    def on_added(self):
        pass


    def on_removed(self):
        pass
    
    def on_modified(self, prop_name):
        pass



class PkgDeviceActor(DeviceActor):
    """ 
    Esta clase encapsula la lógica los actores que se encargan de 
    comprobar e instalar paquetes determinados.

    Es útil para simplicar todo esos actores que lo único que hacen es instalar
    software para que el dispositivo funcione.
    """
    __icon_path__  = ''
    __iconoff_path__ = ''
    __device_title__ = ''
    __device_conn_description__ = ''
    __device_disconn_description__ = ''

    __packages__ = []

    __conn_commands__ = []
    __disconn_commands__ = []


    def __init__(self, message_render, device_properties):
        DeviceActor.__init__(self, message_render, device_properties)
        self.set_default_properties()


    def set_default_properties(self):
        """ Set default properties from config files.
        """
        modname = self.__module__.split('.')[-1]
        config = None
        pkg_path = os.path.abspath('actors/config') + \
                '/' + modname + '.' + PkgDeviceActor._get_desktop()
        try:
            config = syck.load(open(pkg_path).read())
        except Exception, e:
            print e

        if config:
            if not self.__packages__:
                self.__packages__ = config.get('packages', [])
            if not self.__conn_commands__:
                self.__conn_commands__ = config.get('conn_commands', [])
            if not self.__disconn_commands__:
                self.__disconn_commands__ = config.get('disconn_commands', [])


    def get_packages(module_name):
        """ Return packages for an actor.

        Return packages for an actor based on actors/config/* files and on
        user's desktop.
        """
        pkg_path = os.path.abspath('actors/config') + \
                '/' + module_name + '.' + PkgDeviceActor._get_desktop()
        config = {}
        try:
            config = syck.load(open(pkg_path).read())
        except Exception, e:
            pass

        return config.get('packages', [])

    get_packages = staticmethod(get_packages)


    def _get_desktop():
        """ Return desktop name (gnome, or kde)
        """
        # Look for gnome
        gnome_command = 'ps ux|grep gnome-settings|grep -v grep'
        is_gnome = bool(os.popen(gnome_command).read())
        if is_gnome:
            return "gnome"

        # Look for kde
        kde_command = 'ps ux|grep startkde|grep -v grep'
        is_kde = bool(os.popen(kde_command).read())
        if is_kde:
            return "kde"

        # Default packages
        return "gnome"

    _get_desktop = staticmethod(_get_desktop)


    def on_added(self):
        s = PkgInstaller()

        def execute_conn_commands():
            execute = True

            # Obtain sudo permission if needed.
            for command in self.__conn_commands__:
                if command.strip().startswith('sudo'):
                    if not get_sudo():
                        execute = False
                    break

            if execute:
                for command in self.__conn_commands__:
                    os.system(command)

        def install_packages():
            s.install(self.__packages__)
            execute_conn_commands()

        if s.check(self.__packages__):
            if self.__conn_commands__:
                actions = {_("Use device"): execute_conn_commands}
            else:
                actions = {}
        else:
            actions = {_("Install required packages"): install_packages}

        self.msg_render.show(self.__device_title__, 
                self.__device_conn_description__,
             self.__icon_path__, actions = actions)


    def on_removed(self):
        self.msg_render.show(self.__device_title__,
                self.__device_disconn_description__,
                self.__iconoff_path__)
        self._execute_disconn_commands()


    def _install_packages(self):
        s = PkgInstaller()
        s.install(self.__packages__)


    def _execute_conn_commands(self):
        for command in self.__conn_commands__:
            os.system(command)


    def _execute_disconn_commands(self):
        for command in self.__disconn_commands__:
            os.system(command)

