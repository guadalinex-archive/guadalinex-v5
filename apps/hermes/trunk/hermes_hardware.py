#!/usr/bin/python
# -*- coding: utf-8 -*- 

# Authors: 
#     Gumersindo Coronel Pérez (gcoronel) <gcoronel@emergya.es> 
#
# Last modified: 
#     $Date: 2007-08-13 12:12:35 +0000 (Mon, 13 Aug 2007) $ 
#     $Author: gcoronel $

# Módulo hermes_hardware - Notificador de cambios en el hardware
# 
# Copyright (C) 2005 Junta de Andalucía
# 
# Autor/es (Author/s):
# 
# - Gumersindo Coronel Pérez <gcoronel@emergya.info>
# 
# Este fichero es parte de Detección de Hardware de Guadalinex 2005 
# 
# Detección de Hardware de Guadalinex 2005  es software libre. Puede redistribuirlo y/o modificarlo 
# bajo los términos de la Licencia Pública General de GNU según es 
# publicada por la Free Software Foundation, bien de la versión 2 de dicha
# Licencia o bien (según su elección) de cualquier versión posterior. 
# 
# Detección de Hardware de Guadalinex 2005  se distribuye con la esperanza de que sea útil, 
# pero SIN NINGUNA GARANTÍA, incluso sin la garantía MERCANTIL 
# implícita o sin garantizar la CONVENIENCIA PARA UN PROPÓSITO 
# PARTICULAR. Véase la Licencia Pública General de GNU para más detalles. 
# 
# Debería haber recibido una copia de la Licencia Pública General 
# junto con Detección de Hardware de Guadalinex 2005 . Si no ha sido así, escriba a la Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
# 
# -------------------------------------------------------------------------
# 
# This file is part of Detección de Hardware de Guadalinex 2005 .
# 
# Detección de Hardware de Guadalinex 2005  is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# at your option) any later version.
# 
# Detección de Hardware de Guadalinex 2005  is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Foobar; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

def setup_gettext(domain, data_dir):
    directory = os.path.abspath(os.path.join(data_dir, "locale"))
    gettext.bindtextdomain(domain, directory)
    if hasattr(gettext, 'bind_textdomain_codeset'):
        gettext.bind_textdomain_codeset(domain, 'UTF-8')
    gettext.textdomain(domain)

    locale.bindtextdomain(domain, directory)
    if hasattr(locale, 'bind_textdomain_codeset'):
        locale.bind_textdomain_codeset(domain, 'UTF-8')
    locale.textdomain(domain)

import dbus
if getattr(dbus, "version", (0, 0, 0)) >= (0, 41, 0):
    import dbus.glib
import logging
import gtk
import gtk.gdk
import os
import os.path
import sys
import traceback
import types


# Internacionalización
import gettext, locale
from gettext import gettext as _
import defs
setup_gettext('hermes-hardware', defs.DATA_DIR)

from utils.hermestrayicon import HermesTrayIcon
from utils import DeviceList, ColdPlugListener, CaptureLogGui
from optparse import OptionParser
from utils.notification import NotificationDaemon, FileNotification
import actors


# notification-daemon spec: -------------------------------------------
# http://www.galago-project.org/specs/notification/0.9/x408.html#command-notify
# UINT32 org.freedesktop.Notifications.Notify 
#   (STRING app_name, 
#   UINT32 replaces_id, 
#   STRING app_icon, 
#   STRING summary, 
#   STRING body, 
#   ARRAY actions, 
#   DICT hints, 
#   INT32 expire_timeout);

# self.iface.Notify("Hermes", #app_name 
#        0, # replaces_id
#        '', # app_icon
#        '', # summary
#        message, # body
#        '', # actions
#        '', # hints
#        0  #expire_timeout
#        )

class DeviceListener:
    
    def __init__(self, message_render, with_cold = True):
        self.message_render = message_render
        self.logger = logging.getLogger()

        # Inicialize
        self.bus = dbus.SystemBus()

        obj = self.bus.get_object('org.freedesktop.Hal',
                                  '/org/freedesktop/Hal/Manager')

        self.hal_manager = dbus.Interface(obj, 'org.freedesktop.Hal.Manager')

        self.hal_manager.connect_to_signal('DeviceAdded', self.on_device_added)
        self.hal_manager.connect_to_signal('DeviceRemoved', 
                self.on_device_removed)

        self.udi_dict = {}
        self.modify_handler_dict = {}
        self.devicelist = DeviceList()

        self.__init_actors()

        if with_cold:
            coldplug = ColdPlugListener(self)
            coldplug.start()

        self.logger.info("DeviceListener iniciado")


    def on_device_added(self, udi, *args):
        self.logger.debug("DeviceAdded: " + str(udi))
        self.devicelist.save()

        obj = self.bus.get_object('org.freedesktop.Hal', udi)
        obj = dbus.Interface(obj, 'org.freedesktop.Hal.Device')

        properties = obj.GetAllProperties()
        print
        print
        print "#############################################"
        print "CONNECTED ################################"
        print "#############################################"
        self.__print_properties(properties)

        actor = self.get_actor_from_properties(properties)

        if actor: 
            try:
                actor.on_added()
            except:
                self.logger.warning(str(traceback.format_exc()))


    def on_device_removed(self, udi, *args): 
        self.logger.debug("DeviceRemoved: " + str(udi))
        self.devicelist.save()

        if self.udi_dict.has_key(udi):
            disp = self.udi_dict[udi]
            try:
                disp.on_removed()
            except:
                self.logger.warning(str(traceback.format_exc()))
                
            print
            print
            print "#############################################"
            print "DISCONNECTED ################################"
            print "#############################################"
            self.__print_properties(disp.properties)
            del self.udi_dict[udi]
        else:
            self.message_render.show_warning(_("Warning"),
                    _("Device REMOVED."))


    def on_property_modified(self, udi, num, values):
        for ele in values:
            key = ele[0]

            if self.udi_dict.has_key(udi):
                # Actualizamos las propiedades del objeto actor
                actor = self.udi_dict[udi]
                obj = self.bus.get_object('org.freedesktop.Hal', udi)
                obj = dbus.Interface(obj, 'org.freedesktop.Hal.Device')

                actor.properties = obj.GetAllProperties()

                print
                print
                print "#############################################"
                print "MODIFIED PROPERTY:"
                print "udi:", udi
                print key, ':', actor.properties[key]
                print "#############################################"
                try:
                    actor.on_modified(key)
                except Exception, e:
                    self.logger.warning(str(traceback.format_exc()))


    def get_actor_from_properties(self, prop):
        """
        Devuelve un actor que pueda actuar para dispositivos con las propiedades
        espeficicadas en prop
        """
        klass = None
        actor_klass = None

        #    priority ->    1     2     3     4     5
        priority_actors = [None, None, None, None, None]
        priority_counts = [0, 0, 0, 0, 0]

        import actors
        for klass in actors.ACTORSLIST:
            # Set priority to 3 if not in range
            if klass.__priority__ not in (1, 2, 3, 4, 5):
                klass.__priority__ = 3

            kpriority = klass.__priority__ - 1
            count = self.__count_equals(prop, klass.__required__)
            if count > priority_counts[kpriority]:
                priority_counts[kpriority] = count
                priority_actors[kpriority] = klass

        for i in  (4, 3, 2, 1, 0):
	    if  priority_actors[i]: 
		if priority_actors[i].__enabled__:
                    actor_klass = priority_actors[i]
		else: # Enable the actor again for check it in next polling
		   priority_actors[i].__enabled__ = True	
                break

        actor = None 
        udi = prop['info.udi']
        if actor_klass:
            actor = actor_klass(self.message_render, prop)
            self.udi_dict[udi] = actor
            if not self.modify_handler_dict.has_key(udi):
                self.modify_handler_dict[udi] = lambda *args: self.on_property_modified(udi, *args) 
                self.bus.add_signal_receiver(self.modify_handler_dict[udi],
                    dbus_interface = 'org.freedesktop.Hal.Device',
                    signal_name = "PropertyModified",
                    path = udi)
        else:
            # Shorting logger setup (in module actors, logging.getLogger must be
            # invoked _after_ than in main function).
            from actors.deviceactor import DeviceActor
            actor = DeviceActor(self.message_render, prop)
            self.udi_dict[udi] = actor

        return actor


    def __print_properties(self, properties):
        print 
        print 
        print '-----------------------------------------------'
        print "Dispositivo: ", properties['info.udi']
        print 
        keys = properties.keys()
        keys.sort()

        for key in keys:
            print key + ':' + str(properties[key])


    def __count_equals(self, prop, required):
        """
        Devuelve el número de coincidencias entre el diccionario prop y
        required, siempre y cuando TODOS los elementos de required estén en
        prop.
        En caso contrario devuelve 0.
        """
        count = 0
        for key in required.keys():
            if not prop.has_key(key): 
                return 0

            value =  prop[key]
            # Eval required python expressions
            reqvalue = required[key]
            if isinstance(reqvalue,  str) and \
                    reqvalue.strip().startswith('python:'):
                expression = reqvalue.strip()[7:]
                if not eval(expression):
                    return 0

            # Add support for methods and functions
            elif isinstance(reqvalue, types.FunctionType) or \
                    isinstance(reqvalue, types.MethodType):
                if not reqvalue(value):
                    return 0

            else:
                if prop[key] != required[key]:
                    return 0
            count += 1

        return  count


    def __init_actors(self):
        obj = self.bus.get_object('org.freedesktop.Hal', '/org/freedesktop/Hal/Manager')
        manager = dbus.Interface(obj, 'org.freedesktop.Hal.Manager')

        for udi in manager.GetAllDevices():
            obj = self.bus.get_object('org.freedesktop.Hal', udi)
            obj = dbus.Interface(obj, 'org.freedesktop.Hal.Device')

            properties = obj.GetAllProperties()
            self.get_actor_from_properties(properties)


def main():
    try:
        import defs
        setup_gettext('hermes-hardware', defs.DATA_DIR)
    except ImportError:
        print 'WARNING: Running uninstalled, no gettext support'

    # Configure options
    parser = OptionParser(usage = 'usage: %prog [options]')
    parser.set_defaults(debug = False)
    parser.set_defaults(capture_log = False)

    parser.add_option('-d', '--debug', 
            action = 'store_true',
            dest = 'debug',
            help = 'start in debug mode')

    parser.add_option('-c', '--capture-log',
            action = 'store_true',
            dest = 'capture_log',
            help = 'Capture device logs.')

    (options, args) = parser.parse_args()
    del args

    
    # Option debug for logging
    if options.debug:
        level = logging.DEBUG
    else:
        level = logging.INFO

    logfilename = '/var/tmp/hermes-hardware-' + \
            os.environ['USER'] + str(os.getuid()) + \
            '.log' 

    logging.basicConfig(level = level,
            format='%(asctime)s %(levelname)s %(message)s',
                    filename = logfilename,
                    filemode='a')

    # Set capture log
    if options.capture_log:
        filepath = '/var/tmp/filenotification-' + \
                os.environ['USER'] + str(os.getuid()) + \
                '.log'
        iface = FileNotification(filepath)
        capture_log_gui = CaptureLogGui()
    else:
        iface = NotificationDaemon()

    logging.getLogger().info("----------------------------- Hermes init.")

    global DeviceActor
    from actors.deviceactor import DeviceActor

    DeviceListener(iface, with_cold = False)
    HermesTrayIcon()
    gtk.gdk.threads_init()
    try:
        gtk.main()
    except:
        if 'capture_log_gui' in locals():
            # Close file for write in hd.
            capture_log_gui.logfile.close()

        logging.getLogger().info("----------------------------- Hermes finish.")


if __name__ == "__main__":
    main()


