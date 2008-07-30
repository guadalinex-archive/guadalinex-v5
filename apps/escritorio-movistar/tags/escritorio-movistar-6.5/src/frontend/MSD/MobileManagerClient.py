#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#
# Copyright (c) 2003-2008, Telefonica Móviles España S.A.U.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

import os
import sys
import gobject
import time
import dbus
import dbus.glib
import gtk

from MobileManager.MobileStatus import *
from MobileManager.MobileManagerDbus import MOBILE_MANAGER_CONTROLLER_PATH,MOBILE_MANAGER_CONTROLLER_URI,MOBILE_MANAGER_CONTROLLER_INTERFACE_URI,MOBILE_MANAGER_DEVICE_PATH,MOBILE_MANAGER_DEVICE_URI,MOBILE_MANAGER_DEVICE_INFO_INTERFACE_URI,MOBILE_MANAGER_DEVICE_AUTH_INTERFACE_URI,MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI,MOBILE_MANAGER_DEVICE_XZONE_INTERFACE_URI, MOBILE_MANAGER_DIALER_INTERFACE_URI

class MobileManagerControllerClient(gobject.GObject):

    __gsignals__ = { 'active-dev-card-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                         (gobject.TYPE_INT,)) ,
                     'active-dev-tech-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                         (gobject.TYPE_INT,)) ,
                     'active-dev-mode-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                         (gobject.TYPE_INT,)) ,
                     'active-dev-domain-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                           (gobject.TYPE_INT,)) ,
                     'active-dev-signal-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                           (gobject.TYPE_INT,)) ,
                     'active-dev-pin-act-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                            (gobject.TYPE_BOOLEAN,)) ,
                     'active-dev-roaming-status-changed': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                           (gobject.TYPE_BOOLEAN,)) ,
                     'active-dev-carrier-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                     (gobject.TYPE_STRING,)) ,
                     'active-dev-carrier-sm-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                               (gobject.TYPE_INT,)) ,
                     'active-dev-x-zone-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                               (gobject.TYPE_STRING,)) ,

                     'dev-card-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                         (gobject.TYPE_STRING, gobject.TYPE_INT,)) ,
                     'dev-tech-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                         (gobject.TYPE_STRING, gobject.TYPE_INT,)) ,
                     'dev-mode-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                         (gobject.TYPE_STRING, gobject.TYPE_INT,)) ,
                     'dev-domain-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                           (gobject.TYPE_STRING, gobject.TYPE_INT,)) ,
                     'dev-signal-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                          (gobject.TYPE_STRING, gobject.TYPE_INT,)) ,
                     'dev-pin-act-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                            (gobject.TYPE_STRING, gobject.TYPE_BOOLEAN,)) ,
                     'dev-roaming-status-changed': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                           (gobject.TYPE_STRING, gobject.TYPE_BOOLEAN,)) ,
                     'dev-carrier-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                     (gobject.TYPE_STRING, gobject.TYPE_STRING,)) ,
                     'dev-carrier-sm-status-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                               (gobject.TYPE_STRING, gobject.TYPE_INT,)) ,
                     
                     'added-device' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                         (gobject.TYPE_STRING,)) ,
                     'removed-device' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                         (gobject.TYPE_STRING,)) ,
                     'supported-device-detected' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                    (gobject.TYPE_STRING,)) ,
                     
                     'active-device-changed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                                                    (gobject.TYPE_STRING,)) ,
                     
                     }
                     
    def __init__(self):
        gobject.GObject.__init__(self)
        self.dbus = None
        self.mm_manager_obj = None
        self.mcontroller = None
        self.mdialer = None
        self.dialer = None
        self.device_objs = {}
        
        if self.__init_bus() == False:
            return

        self.__connect_signals()

    # Decorators (mobile_manager_crash)
    def mobile_manager_crash():
        def wrap (f):
            def _f(self, *args, **kw):
                try:
                    return f(self, *args, **kw)
                except:
                    self.dbus = None
                    self.mm_manager_obj = None
                    self.mcontroller = None
                    self.mdialer = None
                    self.dialer = None
                    if self.__init_bus() == False:
                        self.__mobile_manager_crash_info_dialog()
                    else:
                        self.__connect_signals()
                        try:
                            return f(self, *args, **kw)
                        except:
                            self.__mobile_manager_crash_info_dialog()
            return _f
        return wrap

    def __mobile_manager_crash_info_dialog(self):
        
        msg = gtk.MessageDialog(parent=None, flags=0,
                                type=gtk.MESSAGE_ERROR,
                                buttons=gtk.BUTTONS_CLOSE, message_format=None)
        msg.set_markup(_(u"<b>Mobile Manager no disponible</b>"))
        msg.format_secondary_markup(_(u"Mobile manager no está activado o no funciona correctamente.\n"
                                      "Por favor trate de reactivarlo para iniciar el Escritorio movistar."))
        ret = msg.run()
        msg.destroy()
        
        sys.exit(0)
        
    def __init_bus(self):
        try:
            self.dbus = dbus.SystemBus()
            
            self.mm_manager_obj = self.dbus.get_object(MOBILE_MANAGER_CONTROLLER_URI,
                                                       MOBILE_MANAGER_CONTROLLER_PATH)
            self.mcontroller = dbus.Interface(self.mm_manager_obj,
                                              MOBILE_MANAGER_CONTROLLER_INTERFACE_URI)
            self.mdialer = dbus.Interface(self.mm_manager_obj,
                                          MOBILE_MANAGER_DIALER_INTERFACE_URI)
        except:
            print _(u"Not dbus connection available")
            return False

        self.dialer = MobileManagerDialerClient(self.mdialer, self.mcontroller)
        return True

    def __connect_signals(self):
        self.mcontroller.connect_to_signal("ActiveDevCardStatusChanged", self.__ActiveDevCardStatusChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevTechStatusChanged", self.__ActiveDevTechStatusChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevModeStatusChanged", self.__ActiveDevModeStatusChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevDomainStatusChanged", self.__ActiveDevDomainStatusChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevSignalStatusChanged", self.__ActiveDevSignalStatusChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevPinActStatusChanged", self.__ActiveDevPinActStatusChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevRoamingActStatusChanged", self.__ActiveDevRoamingActStatusChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevCarrierChanged", self.__ActiveDevCarrierChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevCarrierSmStatusChanged", self.__ActiveDevCarrierSmStatusChanged_cb)
        self.mcontroller.connect_to_signal("ActiveDevXZoneChanged", self.__ActiveDevXZoneChanged_cb)
        self.mcontroller.connect_to_signal("DevCardStatusChanged", self.__DevCardStatusChanged_cb)
        self.mcontroller.connect_to_signal("DevTechStatusChanged", self.__DevTechStatusChanged_cb)
        self.mcontroller.connect_to_signal("DevModeStatusChanged", self.__DevModeStatusChanged_cb)
        self.mcontroller.connect_to_signal("DevDomainStatusChanged", self.__DevDomainStatusChanged_cb)
        self.mcontroller.connect_to_signal("DevSignalStatusChanged", self.__DevSignalStatusChanged_cb)
        self.mcontroller.connect_to_signal("DevPinActStatusChanged", self.__DevPinActStatusChanged_cb)
        self.mcontroller.connect_to_signal("DevRoamingActStatusChanged", self.__DevRoamingActStatusChanged_cb)
        self.mcontroller.connect_to_signal("DevCarrierChanged", self.__DevCarrierChanged_cb)
        self.mcontroller.connect_to_signal("DevCarrierSmStatusChanged", self.__DevCarrierSmStatusChanged_cb)
        self.mcontroller.connect_to_signal("AddedDevice", self.__AddedDevice_cb)
        self.mcontroller.connect_to_signal("RemovedDevice", self.__RemovedDevice_cb)
        self.mcontroller.connect_to_signal("SupportedDeviceDetected", self.__SupportedDeviceDetected_cb)
        self.mcontroller.connect_to_signal("ActiveDeviceChanged", self.__ActiveDeviceChanged_cb)

    def __ActiveDevCardStatusChanged_cb(self, status):
        self.emit('active-dev-card-status-changed', status)
        
    def __ActiveDevTechStatusChanged_cb(self, status):
        self.emit('active-dev-tech-status-changed', status)
        
    def __ActiveDevModeStatusChanged_cb(self, status):
        self.emit('active-dev-mode-status-changed', status)
        
    def __ActiveDevDomainStatusChanged_cb(self, status):
        self.emit('active-dev-domain-status-changed', status)
 
    def __ActiveDevSignalStatusChanged_cb(self, status):
        self.emit('active-dev-signal-status-changed', status)
        
    def __ActiveDevPinActStatusChanged_cb(self, status):
        self.emit('active-dev-pin-act-status-changed' , status)
        
    def __ActiveDevRoamingActStatusChanged_cb(self, status):
        self.emit('active-dev-roaming-status-changed' , status)
        
    def __ActiveDevCarrierChanged_cb(self, carrier_name):
        self.emit('active-dev-carrier-changed' , carrier_name)
        
    def __ActiveDevCarrierSmStatusChanged_cb(self, status):
        self.emit('active-dev-carrier-sm-status-changed' , status)
        
    def __ActiveDevXZoneChanged_cb(self, xzone_name):
        self.emit('active-dev-x-zone-changed' , xzone_name)
    
    def __DevCardStatusChanged_cb(self, device, status):
         self.emit('dev-card-status-changed' , device, status)
        
    def __DevTechStatusChanged_cb(self, device, status):
        self.emit('dev-tech-status-changed' , device, status)
        
    def __DevModeStatusChanged_cb(self, device, status):        
        self.emit('dev-mode-status-changed' , device, status)
        
    def __DevDomainStatusChanged_cb(self, device, status):
        self.emit('dev-domain-status-changed' , device, status)
        
    def __DevSignalStatusChanged_cb(self, device, status):
        self.emit('dev-signal-status-changed' , device, status)
        
    def __DevPinActStatusChanged_cb(self, device, status):
        self.emit('dev-pin-act-status-changed' , device, status)
        
    def __DevRoamingActStatusChanged_cb(self, device, status):
        print "Roaming change ??"
        self.emit('dev-roaming-status-changed' , device, status)
        
    def __DevCarrierChanged_cb(self, device, carrier_name):
        self.emit('dev-carrier-changed' , device, carrier_name)
        
    def __DevCarrierSmStatusChanged_cb(self, device, status):
        self.emit('dev-carrier-sm-status-changed' , device, status)
        
    def __AddedDevice_cb(self, device):
        self.emit('added-device' , device)
                                
    def __RemovedDevice_cb(self, device):
        if self.device_objs.has_key(device) :
            self.device_objs.pop(device)
            print "Remove Device Client of %s" % device
        self.emit('removed-device' , device)
        
    def __SupportedDeviceDetected_cb(self, device):
        self.emit('supported-device-detected' , device)

    def __ActiveDeviceChanged_cb(self, device):
         self.emit('active-device-changed' , device)

    @mobile_manager_crash ()
    def get_active_device(self):
        dev =  self.mcontroller.GetActiveDevice()
        if dev == "":
            return None
        else:
            return dev
        
    @mobile_manager_crash ()  
    def set_active_device(self, path):
        if path == None:
            return False
        return self.mcontroller.SetActiveDevice(path)

    @mobile_manager_crash ()
    def get_available_devices(self):
        return self.mcontroller.GetAvailableDevices()

    @mobile_manager_crash ()
    def get_device_obj_from_path(self, path):
        if self.device_objs.has_key(path) :
            return self.device_objs[path]
        else:
            ret = MobileManagerDeviceClient(self, path)
            self.device_objs[path] = ret
            return ret
    
    
gobject.type_register(MobileManagerControllerClient)

class MobileManagerDeviceClient(gobject.GObject) :
    def __init__ (self, mcontroller, opath) :
        gobject.GObject.__init__(self)
        self.path = opath
        self.mcontroller = mcontroller
        self.dbus = mcontroller.dbus
        
        self.__info = self.__get_device_info_from_path(self.path)
        self.__auth = self.__get_device_auth_from_path(self.path)
        self.__state = self.__get_device_state_from_path(self.path)

        self.cache = {}

        self.mcontroller.connect("dev-roaming-status-changed", self.__DevRoamingActStatusChanged_cb)

    # Decorators (mobile_manager_crash)
    def mobile_manager_crash():
        def wrap (f):
            def _f(self, *args, **kw):
                try:
                    return f(self, *args, **kw)
                except:
                    try:
                        self.path = opath
                        self.mcontroller = mcontroller
                        self.dbus = mcontroller.dbus
        
                        self.__info = self.__get_device_info_from_path(self.path)
                        self.__auth = self.__get_device_auth_from_path(self.path)
                        self.__state = self.__get_device_state_from_path(self.path)

                        self.cache = {}
                        
                        return f(self, *args, **kw)
                    except:
                        self.__mobile_manager_crash_info_dialog()
            return _f
        return wrap

    def __mobile_manager_crash_info_dialog(self):
        
        msg = gtk.MessageDialog(parent=None, flags=0,
                                type=gtk.MESSAGE_ERROR,
                                buttons=gtk.BUTTONS_CLOSE, message_format=None)
        msg.set_markup(_(u"<b>Mobile Manager no disponible</b>"))
        msg.format_secondary_markup(_(u"Mobile manager no está activado o no funciona correctamente.\n"
                                      "Por favor trate de reactivarlo para iniciar el Escritorio movistar."))
        ret = msg.run()
        msg.destroy()
        
        sys.exit(0)

    def __get_device_info_from_path(self, dev_path):
        dev = self.dbus.get_object(MOBILE_MANAGER_DEVICE_URI,
                                   dev_path)
        dev_info = dbus.Interface(dev, MOBILE_MANAGER_DEVICE_INFO_INTERFACE_URI)
        return dev_info
    
    def __get_device_auth_from_path(self, dev_path):
        dev = self.dbus.get_object(MOBILE_MANAGER_DEVICE_URI,
                                   dev_path)
        dev_auth = dbus.Interface(dev, MOBILE_MANAGER_DEVICE_AUTH_INTERFACE_URI)
        return dev_auth

    def __get_device_state_from_path(self, dev_path):
        dev = self.dbus.get_object(MOBILE_MANAGER_DEVICE_URI,
                                   dev_path)
        dev_state = dbus.Interface(dev, MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI)
        return dev_state

    # INFO INTERFACE
    @mobile_manager_crash ()    
    def get_capabilities(self):
        if self.cache.has_key("capabilities") :
            return self.cache["capabilities"]
        else:
            ret = self.__info.GetCapabilities()
            self.cache["capabilities"] = ret
            return ret

    @mobile_manager_crash ()
    def has_capability(self, capability):
        if self.cache.has_key("capabilities") :
            if capability in self.cache["capabilities"]:
                return True
            else:
                return False
        else:
            ret = self.__info.GetCapabilities()
            self.cache["capabilities"] = ret
            
        return self.__info.HasCapability(capability)

    @mobile_manager_crash ()   
    def get_velocity(self):
        if self.cache.has_key("velocity") :
            return self.cache["velocity"]
        else:
            ret = self.__info.GetVelocity()
            self.cache["velocity"] = ret
            return ret

    @mobile_manager_crash ()
    def get_prettyname(self):
        if self.cache.has_key("prettyname") :
            return self.cache["prettyname"]
        else:
            ret = self.__info.GetPrettyName()
            self.cache["prettyname"] = ret
            return ret

    @mobile_manager_crash ()
    def get_priority(self):
        if self.cache.has_key("priority") :
            return self.cache["priority"]
        else:
            ret = self.__info.GetPriority()
            self.cache["priority"] = ret
            return ret

    @mobile_manager_crash ()
    def set_prority(self, priority):
        self.cache["priority"] = priority
        return self.__info.SetPriority(priority)
    
        
    # STATUS INTERFACE

    @mobile_manager_crash ()
    def get_card_info(self):
        if self.cache.has_key("cardinfo") :
            return self.cache["cardinfo"]
        else:
            ret = self.__state.GetCardInfo()
            self.cache["cardinfo"] = ret
            return ret

    @mobile_manager_crash ()
    def get_card_status(self):
        return self.__state.GetCardStatus()

    @mobile_manager_crash ()
    def turn_off(self):
        return self.__state.TurnOff()

    @mobile_manager_crash ()
    def turn_on(self):
        return self.__state.TurnOn()

    @mobile_manager_crash ()
    def is_on(self):
        return self.__state.IsOn()

    def get_cover_key(self, rfunc, efunc):
        self.__state.GetUSSDCmd('at+cusd=1,"*127*0*5#",15',
                                reply_handler=rfunc,
                                error_handler=efunc)

    @mobile_manager_crash ()
    def is_roaming(self):
        if (os.path.exists(os.path.join(os.environ["HOME"], ".movistar_desktop/roaming"))) :
            return True
        
        if self.cache.has_key("roaming") :
            return self.cache["roaming"]
        else:
            if self.has_capability(MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI) :
                ret = self.__state.IsRoaming()
                self.cache["roaming"] = ret
                return ret
            else:
                self.cache["roaming"] = False
                return False

    def __DevRoamingActStatusChanged_cb(self, mcontroller, device, status):
        if self.path == device :
            self.cache["roaming"] = status
            print "Caching roaming value (%s , %s, %s)" % (self, device, status)

class MobileManagerDialerClient(gobject.GObject):

    __gsignals__ = { 'connecting' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,()) ,
                     'connected' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,()) ,
                     'disconnecting' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,()) ,
                     'disconnected' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,()),
                     'pppstats_signal' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,(gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_FLOAT))
                     }
    
    def __init__(self, mdialer, mcontroller):
        self.mdialer = mdialer
        self.mcontroller = mcontroller
        self.stop_delegates = []
        self.mdialer.connect_to_signal("Connected", self.__connected_cb)
        self.mdialer.connect_to_signal("Connecting", self.__connecting_cb)
        self.mdialer.connect_to_signal("Disconnected", self.__disconnected_cb)
        self.mdialer.connect_to_signal("Disconnecting", self.__disconnecting_cb)
        self.mdialer.connect_to_signal("Stats", self.__stats_cb)
        self.proxy_original_values = {}
        
        gobject.GObject.__init__(self)

    # Decorators (mobile_manager_crash)
    def mobile_manager_crash():
        def wrap (f):
            def _f(self, *args, **kw):
                try:
                    return f(self, *args, **kw)
                except:
                    self.__mobile_manager_crash_info_dialog()
            return _f
        return wrap

    def __mobile_manager_crash_info_dialog(self):
        
        msg = gtk.MessageDialog(parent=None, flags=0,
                                type=gtk.MESSAGE_ERROR,
                                buttons=gtk.BUTTONS_CLOSE, message_format=None)
        msg.set_markup(_(u"<b>Mobile Manager no disponible</b>"))
        msg.format_secondary_markup(_(u"Mobile manager no está activado o no funciona correctamente.\n"
                                      "Por favor trate de reactivarlo para iniciar el Escritorio movistar."))
        ret = msg.run()
        msg.destroy()
        
        sys.exit(0)

    @mobile_manager_crash ()
    def start(self, parameters):
        #        PARAMS : {'SECUNDARY_DNS': '194.179.1.101', 'USE_PROXY': 'NO', 'PASWORD': 'MOVISTAR', 'PRIMARY_DNS': '194.179.1.100', 'CYPHER_PASSWORD': 'NO', 'PROXY_PORT': '', 'PROXY_ADDRESS': '', 'DNS_SUFFIX': '', 'APN': '', 'LOGIN': 'MOVISTAR', 'AUTO_DNS': 'NO'}

        user = parameters["LOGIN"]
        passwd = parameters["PASWORD"]
        apn = parameters["APN"]
        proxy_host = parameters["PROXY_ADDRESS"]
        proxy_port = parameters["PROXY_PORT"]
        
        auto_dns = True
        pdns = ""
        sdns = ""
        dnssufix = ""
        
        if parameters["AUTO_DNS"] == "NO" :
            auto_dns = False
            pdns = parameters["PRIMARY_DNS"]
            sdns = parameters["SECUNDARY_DNS"]
            dnssufix =  parameters["DNS_SUFFIX"]

        if apn == "" :
            apn = "movistar.es"
        

        print _(u"Parameters %s ") % parameters
        print _(u"Parameters %s ,%s, %s ,%s, %s, %s, %s") % (user, passwd, apn, auto_dns, pdns, sdns, dnssufix)
        
        self.mdialer.Start(user, passwd, apn, auto_dns, pdns, sdns, dnssufix)

        keys = ['host', 'port', 'use_http_proxy']
        self.proxy_original_values = {} 
        
        for key in keys:
            ret = os.popen("gconftool-2 -g /system/http_proxy/%s" % key)
            self.proxy_original_values[key] = ret.readline().strip("\n")

        print self.proxy_original_values
        
        if proxy_host != "":
            os.system("gconftool-2 -t string -s /system/http_proxy/host %s" % str(proxy_host))
            os.system("gconftool-2 -t bool -s /system/http_proxy/use_http_proxy 'true'")

        if proxy_port != "":
            os.system("gconftool-2 -t int -s /system/http_proxy/port %s" % str(proxy_port))
                      
        print parameters

    
    def stop(self):
        print _(u"MobileDial stop")
        for func in self.stop_delegates :
            print func
            func()
            
        os.system("gconftool-2 -t string -s /system/http_proxy/host %s" % str(self.proxy_original_values["host"]))
        os.system("gconftool-2 -t int -s /system/http_proxy/port %s" % str(self.proxy_original_values["port"]))
        os.system('gconftool-2 -t bool -s /system/http_proxy/use_http_proxy %s' % str(self.proxy_original_values["use_http_proxy"]))
        
        self.mdialer.Stop()

    @mobile_manager_crash ()
    def status(self):
        return self.mdialer.Status()

    def add_stop_delegate(self, func):
        self.stop_delegates.append(func)

    def __connected_cb(self):
        self.emit("connected")

    def __connecting_cb(self):
        self.emit("connecting")

    def __disconnected_cb(self):
        self.emit("disconnected")

    def __disconnecting_cb(self):
        self.emit("disconnecting")

    def __stats_cb(self, r_bytes, s_bytes, interval_time):
        self.emit("pppstats_signal", r_bytes, s_bytes, interval_time)
        

gobject.type_register(MobileManagerDialerClient)
    

    

    
