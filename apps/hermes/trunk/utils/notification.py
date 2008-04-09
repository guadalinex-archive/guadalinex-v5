# -*- coding: utf-8 -*-

import dbus
import thread
import logging
import os
import time

if getattr(dbus, "version", (0, 0, 0)) >= (0, 41, 0):
    import dbus.glib


class NotificationDaemon(object):
    """
    This class is a wrapper for notification-daemon program.
    """

    def __init__(self): 
        self.logger = logging.getLogger()
        bus = dbus.SessionBus()
        obj = bus.get_object('org.freedesktop.Notifications',
                '/org/freedesktop/Notifications')

        self.iface = dbus.Interface(obj, 'org.freedesktop.Notifications')

    # Main Message #######################################################

    def show(self, summary, message, icon, actions = {}): 
        if actions != {}:
            timeout = 12000
            (notify_actions,action_handlers) = self.__process_actions(actions)
            
            def action_invoked(nid, action_id):
                if action_handlers.has_key(action_id) and res == nid:
                    #Execute the action handler
                    thread.start_new_thread(action_handlers[action_id], ())

                self.iface.CloseNotification(dbus.UInt32(nid))

            condition = False
            while not condition:
                try:
                    self.logger.debug("Trying to connect to ActionInvoked")
                    self.iface.connect_to_signal("ActionInvoked", action_invoked)
                    condition = True
                except:
                    logmsg = "ActionInvoked handler not configured. "
                    logmsg += "Trying to run notification-daemon."
                    self.logger.warning(logmsg)
                    os.system('/usr/lib/notification-daemon/notification-daemon &')                
                    time.sleep(0.2)

        else:
            timeout = 7000
            #Fixing no actions messages
            notify_actions = []

        res = self.iface.Notify("Hermes", 
        dbus.UInt32(0),
                dbus.String(icon),
                summary, 
                message, 
                notify_actions,
        {},
                dbus.UInt32(timeout))
        return res


    # Specific messages #################################

    def show_info(self, summary, message, actions = {}):
        return self.show(summary, message, "gtk-dialog-info", actions)


    def show_warning(self, summary, message, actions = {}):
        return self.show(summary, message, "gtk-dialog-warning", actions)


    def show_error(self, summary, message, actions = {}):
        return self.show(summary, message, "gtk-dialog-error", actions)


    def close(self, nid):
        try:
            self.iface.CloseNotification(dbus.UInt32(nid))
        except:
            pass

# Private methods ###################################
    def __process_actions(self, actions):
        """
        Devuelve una 2-tupla

        La primera es una lista cuyos valores pares (comenzando por 0)
    se refieren a la identificación de la acción y cuyos valores
    impares serán la cadena a mostrar en el botón de la acción

        El segundo contiene como claves los identificadores (enteros) de las
        acciones a tomar y como valores las funciones a ejecutar
        """
        if actions == {}:
            #FIXME
            return {}, {}

        notify_actions = []
        action_handlers = {}
        i = 0
        for key, value in actions.items():
            notify_actions.append(dbus.String(i))
            notify_actions.append(key)
            action_handlers[dbus.String(i)] = value
            i += 1

        return notify_actions, action_handlers

class FileNotification(object):

    def __init__(self, filepath):
        self.filepath = filepath


    def show (self, summary, body, icon, actions = {}):
        self.__write("show: %s, %s, %s" % (summary, body, icon))


    def show_info(self, summary, body, actions = {}):
        self.__write("show_info: %s, %s, %s" % (summary, body))


    def show_warning(self, summary, body, actions = {}):
        self.__write("show_warning: %s, %s, %s" % (summary, body))


    def show_error(self, summary, body, actions = {}):
        self.__write("show_error: %s, %s, %s" % (summary, body))


    def __write(self, text):
        try:
            objfile = open(self.filepath, 'a')
            objfile.write(text + '\n')
            objfile.close()
        except Exception, e:
            logging.getLogger().error('FileNotification: ' + e)
