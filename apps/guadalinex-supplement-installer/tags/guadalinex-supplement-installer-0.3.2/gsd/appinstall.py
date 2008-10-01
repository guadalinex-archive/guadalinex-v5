#!/usr/bin/python
# -*- coding: utf-8 -*-

# Copyright (C) 2004-2005 Ross Burton <ross@burtonini.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA 02111-1307 USA
import sys, gettext, os

import pygtk; pygtk.require("2.0")
import gtk, gtk.glade
import dbus

sys.path.insert(0, '/usr/share/gsd')
import config
import gsdutils
from gsdutils import SupplementCustomizer

gettext.bindtextdomain("gnome-app-install", "/usr/share/locale")
gettext.textdomain("gnome-app-install")
gtk.glade.bindtextdomain("gnome-app-install", "/usr/share/locale")
gtk.glade.textdomain("gnome-app-install")


class DiskSelectorDialog(gtk.Dialog):

    def __init__(self, devices):
        gtk.Dialog.__init__(self, "Seleccione disco de suplementos",
                None, 
                buttons = (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                gtk.STOCK_OK, gtk.RESPONSE_OK)
                )

        self.devices = devices
        self._configure()
        self.show_all()


    def get_selected(self):
        index = self.combobox.get_active()
        return self.devices[index]


    def _configure(self):
        self.set_default_response(gtk.RESPONSE_OK)

        # Label
        msg = '<b> Seleccione el disco de suplementos </b>'
        label = gtk.Label()
        label.set_markup(msg)
        self.vbox.pack_start(label)
        
        # ComboBox
        self.combobox = gtk.combo_box_new_text()
        for device in self.devices:
            self.combobox.append_text(device['volume.label'])
        if len(self.devices) > 0:
            self.combobox.set_active(0)

        self.vbox.pack_start(self.combobox)


class Finder (object):

    def __init__(self):
        self.bus = dbus.SystemBus()
        obj = self.bus.get_object('org.freedesktop.Hal',
                                  '/org/freedesktop/Hal/Manager')

        self.hal_manager = dbus.Interface(obj, 'org.freedesktop.Hal.Manager')


    def get_mount_point(self):
        devices = self.get_devices()

        if len(devices) == 1:
            device = devices[0]
        elif len(devices) > 1:
            device = self._select_device(devices)
        else:
            # Not device found
            msg = '<b>No se ha encontrado ningún disco de suplementos.</b>\n\n'
            msg += 'Si está intentando instalar un suplemento desde un CD-ROM '
            msg += 'o DVD, asegúrese de que está presente en el lector.'
            dialog = gtk.MessageDialog(type = gtk.MESSAGE_ERROR,
                    buttons = gtk.BUTTONS_CLOSE,
                    message_format = '')
            dialog.set_markup(msg)
            dialog.set_title('Suplemento no encontrado')
            dialog.run()
            dialog.destroy()
            sys.exit(1)
        return device.get('volume.mount_point', '')


    def _select_device(self, devices):
        # Show dialog for device selection
        dialog = DiskSelectorDialog(devices)
        response = dialog.run()
        if response == gtk.RESPONSE_OK:
            dialog.destroy()
            return dialog.get_selected()
        else:
            sys.exit(0)


    def get_devices(self):
        devices = []
        for udi in  self.hal_manager.GetAllDevices():
            devobj = self.bus.get_object('org.freedesktop.Hal', udi)
            devobj = dbus.Interface(devobj, 'org.freedesktop.Hal.Device')
            properties = devobj.GetAllProperties()

            if properties.get('info.category', None) == 'volume' \
               and config.is_valid_label(properties.get('volume.label', None)):
                devices.append(properties)

        return devices



def main(default_mountpoint = None):
    if os.geteuid() != 0:
        _ = gettext.gettext
        msg = 'Necesita ejecutar este programa como root '
        msg += '(por ejemplo, con <i>sudo guadalinex-app-install</i>'
        msg += ' desde un terminal).'

        dialog = gtk.MessageDialog(None, 0, gtk.MESSAGE_ERROR,
                       gtk.BUTTONS_OK)
        dialog.set_markup(msg)
        dialog.set_title('Error')
        dialog.run()
        dialog.destroy()
        sys.exit(1)

    if len(sys.argv) > 1:
        mount_point = sys.argv[1]
    elif default_mountpoint:
        mount_point = default_mountpoint
    else:
        finder = Finder()
        mount_point = finder.get_mount_point()

    sys.path.insert(0, "/usr/lib/gnome-app-install")
    os.environ['APT_CONFIG'] = gsdutils.APTCONFPATH

    # Hooks
    cmd = mount_point + '/hook/preinstall'
    if os.path.exists(cmd):
        os.system('sh ' + cmd)

    from GSDAppInstall import AppInstall
    desktop_folder = os.path.join(mount_point,"guadalinex-suplementos-apps")
    suppc = SupplementCustomizer(mount_point)
    suppc.customize()
    app = AppInstall("/usr/share/gsd", 
            desktop_folder, [sys.argv[0]] + sys.argv[2:])
    gtk.main()


