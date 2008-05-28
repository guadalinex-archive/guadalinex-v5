#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
sys.path.append('../')

import unittest
import dbus
import dbus.service
import thread
import threading
import gtk
import logging
import os
import os.path


logfilename = '/var/tmp/hermes-hardware-' + \
            os.environ['USER'] + str(os.getuid()) + \
            '.log' 

logging.basicConfig(level = logging.DEBUG,
           format='%(asctime)s %(levelname)s %(message)s',
           filename = logfilename,
           filemode='a')

DIR = os.path.dirname(__file__) + os.sep 


class HalDevice(dbus.service.Object):

    def __init__(self, bus_name, object_path, properties):
        dbus.service.Object.__init__(self, bus_name, object_path)
        #self.properties = properties

    @dbus.service.method('org.freedesktop.Hal.Device')
    def GetAllProperties(self):
        return self.properties



class VirtualHal(object):
    instance = None
    id = 0

    @staticmethod
    def get_instance():
        if not VirtualHal.instance:
            vhi = VirtualHal()
            VirtualHal.instance = vhi 
            gtk.threads_init()
            thread.start_new_thread(gtk.main, ())
        return VirtualHal.instance


    def __init__(self, name):
        #session_bus = dbus.SessionBus()
        #name = dbus.service.BusName("org.freedesktop.Hal", 
        #    bus = session_bus)
        modules = self.__scan_for_modules()
        self.devices = []
        for mod in modules:
            udi = '/org/freedesktop/Hal/devices/test_' + \
                    str(VirtualHal.id)
            mod.PROPERTIES['info.udi'] = udi
            VirtualHal.id += 1
            new_device = HalDevice(name, udi, mod.PROPERTIES)
            self.devices.append(new_device)


    def __scan_for_modules(self):
        file_list = [ele for ele in os.listdir(DIR) if os.path.isfile(DIR + os.sep + ele)]
        dev_list = [ele for ele in file_list if (ele.startswith('dev_') and \
                                          ele.endswith('.py'))]
        modules_name = [filename.split('.')[0] for filename in dev_list]
        modules = [__import__(module_name, globals(), locals(),['*']) \
                    for module_name in modules_name]

        return modules

if __name__ == '__main__':
    session_bus = dbus.SessionBus()
    name = dbus.service.BusName("org.freedesktop.Hal", 
           bus = session_bus)
    object = HalDevice(name, '/org/freedesktop/Hal/devices/test_0', {}) 
    gtk.main()

