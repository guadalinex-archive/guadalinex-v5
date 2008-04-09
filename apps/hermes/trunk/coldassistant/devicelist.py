# -*- coding: utf-8 -*-

import gtk
import dbus
import os
import thread
import threading
import time

from gettext import gettext as _

from actors import ACTORSLIST
from actors.deviceactor import DeviceActor
from actors.deviceactor import PkgDeviceActor
from actors import usbdevice
from utils.notification import FileNotification
from hermes_hardware import DeviceListener


class DeviceList(gtk.VBox):
    """ DeviceList controller. 
    
    It is a treeview with a liststorage for keep information 
    about Hermes actors and devices.
    """
    __instance__ = None

    def __init__(self):
        gtk.VBox.__init__(self, spacing = 10)


    @staticmethod
    def get_instance():
        if not DeviceList.__instance__:
            DeviceList.__instance__ = DeviceList()
        return DeviceList.__instance__


    def reset(self):
        for child in self.get_children():
            self.remove(child)        

        self._populate()


    def _populate(self):
        while gtk.events_pending():
            gtk.main_iteration()

        device_listener = DeviceListener(FileNotification('/dev/null'), with_cold = False)

        bus = dbus.SystemBus()
        obj = bus.get_object('org.freedesktop.Hal', '/org/freedesktop/Hal/Manager')
        manager = dbus.Interface(obj, 'org.freedesktop.Hal.Manager')
        progressbar = gtk.ProgressBar()
        progressbar.set_fraction(0)
        progressbar.set_text(_('Searching for devices...'))

        self.pack_start(progressbar)
        self.show_all()
        while gtk.events_pending():
            gtk.main_iteration()

        good_actors = []
        
        for udi in manager.GetAllDevices():
            obj = bus.get_object('org.freedesktop.Hal', udi)
            obj = dbus.Interface(obj, 'org.freedesktop.Hal.Device')
            properties = obj.GetAllProperties()
            actor = device_listener.get_actor_from_properties(properties)
            if actor and self._actor_is_good(actor):
                good_actors.append(actor)

        total = len(good_actors)
        i = 1
        actor_renderers = []
        if not good_actors:
            self.remove(progressbar)
            msg = _("Sorry, can't find any device with actions.")
            label = gtk.Label(msg)
            label.set_line_wrap(True)
            self.pack_start(label)
            self.show_all()
            return

        for good_actor in good_actors:
            progressbar.set_fraction(i / float(total))
            while gtk.events_pending():
                gtk.main_iteration()
            i += 1
            actor_renderers.append(ActorRenderer(good_actor))

        self.remove(progressbar)
        for renderer in actor_renderers:
            self.pack_start(renderer)
        self.show_all()


    def _actor_is_good(self, actor):
        """
        Actor filtering.

        Say if actor has associated actions.
        """
        if actor.__dict__.get('__packages__', None):
            return True

        if issubclass(actor.__class__, PkgDeviceActor):
            return True

        return False



class  ActorRenderer(gtk.VBox):

    def __init__(self, actor):
        gtk.VBox.__init__(self, spacing = 10)

        self.label = gtk.Label()
        self.image = gtk.Image()
        self.actor = actor
        self.hbox = gtk.HBox()

        self._configure()
        self.show_all()


    def _configure(self):
        actor = self.actor.__class__(self, self.actor.properties)
        actor.on_added()


    # Notification interface #######################################################

    def show (self, summary, body, icon, actions = {}):
        if not actions:
            return 

        # Icon
        if os.sep in icon:
            self.image.set_from_file(icon)
        else:
            self.image.set_from_stock(icon, gtk.ICON_SIZE_DIALOG)

        self.hbox.pack_start(self.image, False, False)

        # Text
        self.label.set_text(summary)
        self.hbox.pack_start(self.label, False, False)

        # Actions
        action_vbox = gtk.VBox()
        for text, action in actions.items():
            actionrenderer = ActionRenderer(text, action)
            action_vbox.pack_start(actionrenderer, True, False)

        self.hbox.pack_start(action_vbox, True, False)

        self.pack_start(self.hbox)
        self.pack_start(gtk.HSeparator())


    def show_info(self, summary, body, actions = {}):
        self.show(summary, body, gtk.STOCK_DIALOG_INFO, actions)


    def show_warning(self, summary, body, actions = {}):
        self.show(summary, body, gtk.STOCK_DIALOG_WARNING, actions)


    def show_error(self, summary, body, actions = {}):
        self.show(summary, body, gtk.STOCK_DIALOG_ERROR, actions)



class ActionRenderer(gtk.Button):

    def __init__(self, text, action):
        gtk.Button.__init__(self, text)
        
        self.connect('clicked', self.on_clicked)
        self.action = action

        self.show_all()


    def in_thread(self, event):
        os.system('touch /tmp/in_thread')
        os.system('echo in_thread >> /tmp/in_thread')
        self.action()
        event.set()


    def on_clicked(self, widget):
        # Change label for a progress bar.
        pulse_bar = gtk.ProgressBar()
        pulse_bar.set_pulse_step(0.2)
        pulse_bar.show_all()
        label = self.get_child()
        self.remove(label)
        self.add(pulse_bar)
        pulse_bar.pulse()
        self.set_sensitive(False)

        # start thread
        event = threading.Event()
        thread.start_new_thread(self.in_thread, (event, ))
        while not event.isSet():
            while gtk.events_pending():
                gtk.main_iteration()
            pulse_bar.pulse()
            time.sleep(0.2)

        #self.remove(pulse_bar)
        #self.add(label)
        #self.set_sensitive(True)
        gtk.main_quit()
