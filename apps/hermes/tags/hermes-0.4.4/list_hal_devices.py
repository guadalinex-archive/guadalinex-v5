#!/usr/bin/python
# -*- coding: utf8 -*-

import dbus

bus = dbus.SystemBus()

obj = bus.get_object('org.freedesktop.Hal', '/org/freedesktop/Hal/Manager')
manager = dbus.Interface(obj, 'org.freedesktop.Hal.Manager')

for udi in manager.GetAllDevices():
    obj = bus.get_object('org.freedesktop.Hal', udi)
    obj = dbus.Interface(obj, 'org.freedesktop.Hal.Device')
    properties = obj.GetAllProperties()

    print 
    print
    print "################################################"
    print "DEVICE: ", udi
    print
    for key, value in properties.items():
        print key, ':', value
    print "################################################"

    


