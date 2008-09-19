# -*- coding: utf8 -*-

import dbus
import pickle
import logging
import os
import pwd
import time

from sets import Set

class DeviceList:
    """
    Esta clase implementa toda la lógica necesaria para comprobar si se han
    añadido/quitado dispositivos "en frío"

    Su uso es el siguiente:

    dl = DeviceList() #Con esto inicializamos la lista de dispositivos con los
    encontrados en el sistema

    dl.save('/tmp/dl') #Con esto guardamos la lista en un fichero

    # ...

    #Tras arrancar la máquina:

    dl = DeviceList()

    dl.set_file_to_compare('/tmp/dl') #Le comunica a DeviceList que use ese
    #fichero para comparar los dispositivos

    dl.get_added() #Obtenemos una lista con los dispositivos añadidos. El
    #formato de esta lista se encuentra en la documentación del método

    dl.get_removed() #Obtenemos una lista con los dispositivos quitados.

    Nota: Para agilizar esta lógica, se utiliza por defecto el fichero cuyo
    nombre está en DeviceList.DEFAULT_FILE para todas las operaciones con
    ficheros, cargándose, si existe previamente, la lista de dispositivos nada
    más inicializar el objeto DeviceList:

    El ejemplo anterior:

    dl = DeviceList()
    dl.save()

    dl = DeviceList()
    dl.get_added()
    dl.get_removed()
    """

    DEFAULT_FILE = '/var/tmp/devicelist-file-' + \
            os.environ['USER'] + \
            str(os.getuid())

    def __init__(self):
        self.logger = logging.getLogger()
        self.__udi_set = Set()
        self.__properties_dict = {}
        self.__data = (self.__udi_set, self.__properties_dict)
        self.__data_to_compare = None

        self.__setup()
        self.set_file_to_compare()


    def save(self, filename = DEFAULT_FILE):
        try:
            file = open(filename, 'w')
            pickle.dump(self.__data, file)
            file.close()
        except Exception, e:
            self.logger.error(str(e))


    def set_file_to_compare(self, filename = DEFAULT_FILE):
        try:
            file = open(filename, 'r')
        except IOError:
            self.logger.warning("Creating " + filename)
            self.save(filename)
            file = open(filename, 'r')

        try:
            self.__data_to_compare = pickle.load(file)
            file.close()
        except EOFError, e:
            self.__data_to_compare = {0:Set([])}
            self.logger.warning("Error reading from: " + filename)



    def get_added(self):
        """
        Returns a list of tuples (udi,properties) contains the devices added
        data.
        """
        system_udis = self.__data[0]
        store_udis = self.__data_to_compare[0]

        udis_added = system_udis - store_udis

        res = []
        for udi in udis_added:
            res.append((udi, self.__data[1][udi]))

        return res


    def get_removed(self):
        """
        Returns a list of tuples (udi,properties) contains the devices removed
        data.
        """
        system_udis = self.__data[0]
        store_udis = self.__data_to_compare[0]

        udis_removed = store_udis - system_udis 

        res = []
        for udi in udis_removed:
            res.append((udi, self.__data_to_compare[1][udi]))

        return res


    def __setup(self):
        bus = dbus.SystemBus()
        obj = bus.get_object('org.freedesktop.Hal', '/org/freedesktop/Hal/Manager')
        manager = dbus.Interface(obj, 'org.freedesktop.Hal.Manager')

        for udi in manager.GetAllDevices():
            devobj = bus.get_object('org.freedesktop.Hal', udi)
            devobj = dbus.Interface(devobj, 'org.freedesktop.Hal.Device')

            self.__udi_set.add(udi)
            self.__properties_dict[udi] = devobj.GetAllProperties()


