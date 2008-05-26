# MSDeviceManager.py

# Copyright (c) 2007, Junta de Andalucia

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import sys
import os
import gobject
import gtk
import dbus
if getattr(dbus, "version", (0,0,0)) >= (0,41,0):
    import dbus.glib

class MSDeviceManager(gobject.GObject):

    __gsignals__ = {
        'volume_added' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,gobject.TYPE_STRING,gobject.TYPE_STRING,)),
        'volume_removed' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,)),
        'volume_mounted' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,gobject.TYPE_STRING,gobject.TYPE_STRING,)),
        'volume_unmounted' : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,))
        }
    
    def __init__(self):
        gobject.GObject.__init__(self)
        self.dbus = dbus.SystemBus()
        self.hal_manager_obj = self.dbus.get_object("org.freedesktop.Hal", "/org/freedesktop/Hal/Manager")
        self.hal_manager = dbus.Interface(self.hal_manager_obj, "org.freedesktop.Hal.Manager")

        self.volumes = []
        self.storages = []

        # first time
        self.__first_time_device_list()

        # signals
        self.hal_manager.connect_to_signal("DeviceAdded", self.__plug_cb)
        self.hal_manager.connect_to_signal("DeviceRemoved", self.__unplug_cb)

    def __plug_cb(self, uid):
        device_dbus_obj = self.dbus.get_object("org.freedesktop.Hal", uid)
        properties = device_dbus_obj.GetAllProperties(dbus_interface="org.freedesktop.Hal.Device")

        try:
            if "storage" in properties["info.capabilities"]:
                if properties["storage.bus"] == "usb":
                    if "portable_audio_player" in properties["info.capabilities"]:
                        storage_type = "media-player"
                    else:
                        storage_type = "usb"
                    self.storages.append({"uid" : uid,
                                          "human_name": properties["info.vendor"] + " " + properties["info.product"],
                                          "type": storage_type
                                          })

            elif properties["info.category"] == "volume":
                for storage in self.storages:
                    if properties["block.storage_device"] == storage["uid"]:
                        if properties["volume.label"] != "":
                            human_name = properties["volume.label"]
                        else:
                            human_name = storage["human_name"]
                        self.volumes.append({"uid" : uid,
                                             "human_name" : human_name,
                                             "type": storage["type"],
                                             "mount_point": properties["volume.mount_point"],
                                             "is_mounted": properties["volume.is_mounted"]
                                             })
                        extras = {'real_uid': uid}
                        self.dbus.add_signal_receiver(lambda *args: apply (self.__property_modified, args, extras),
                                                      "PropertyModified",
                                                      "org.freedesktop.Hal.Device",
                                                      "org.freedesktop.Hal",
                                                      uid)
                        self.emit("volume_added", uid, human_name, storage["type"])
            
        except:
            pass

    def __unplug_cb(self, uid):
        if (self.__storage_exists (uid)):
            self.__storage_remove (uid)
            
        if (self.__volume_exists (uid)):
            self.__volume_remove (uid)
            self.emit('volume_removed', uid)

    def __property_modified(self, nb_props, change_list, **kw):
        uid = kw.get('real_uid')
        
        for i in change_list:
	    property_name = i[0]

            if property_name == "volume.is_mounted":
                device_dbus_obj = self.dbus.get_object("org.freedesktop.Hal", uid)
                properties = device_dbus_obj.GetAllProperties(dbus_interface="org.freedesktop.Hal.Device")
                volume = self.get_volume(uid)
                if properties["volume.is_mounted"] == 0:
                    if volume["is_mounted"] == 1:
                        volume["is_mounted"] = 0
                        self.emit('volume_unmounted', uid)
                else:
                    if volume["is_mounted"] == 0:
                        volume["is_mounted"] = 1
                        volume["mount_point"] = properties["volume.mount_point"]
                        self.emit('volume_mounted', uid, volume["human_name"], volume["type"])

    def __volume_exists (self, uid):
        for volume in self.volumes:
            if volume["uid"] == uid:
                return True

        return False

    def __volume_remove (self, uid):
        for volume in self.volumes:
            if volume["uid"] == uid:
                self.volumes.remove(volume)
                return

    def __storage_exists (self, uid):
        for storage in self.storages:
            if storage["uid"] == uid:
                return True

        return False

    def __storage_remove (self, uid):
        for storage in self.storages:
            if storage["uid"] == uid:
                self.storages.remove(storage)
                return
            
    def __first_time_device_list(self):
        storages_uids = self.hal_manager.FindDeviceByCapability('storage')
        for storage_uid in storages_uids:
            device_dbus_obj = self.dbus.get_object("org.freedesktop.Hal", storage_uid)
            properties = device_dbus_obj.GetAllProperties(dbus_interface="org.freedesktop.Hal.Device")
            if properties["storage.bus"] == "usb":
                self.storages.append({"uid" : storage_uid,
                                      "human_name": properties["info.vendor"] + " " + properties["info.product"],
                                      "type": "usb",
                                      })
            elif properties["storage.drive_type"] == "floppy":
                self.storages.append({"uid" : storage_uid,
                                      "human_name": properties["info.product"],
                                      "type": "floppy",
                                      })
            else:
                for capability in properties["info.capabilities"]:
                    if capability == "storage.cdrom":
                        self.storages.append({"uid" : storage_uid,
                                      "human_name": properties["info.product"],
                                      "type": "cdrom",
                                      })

        volumes_names = self.hal_manager.FindDeviceByCapability('volume')
        for volume_uid in volumes_names:
            device_dbus_obj = self.dbus.get_object("org.freedesktop.Hal", volume_uid)
            properties = device_dbus_obj.GetAllProperties(dbus_interface="org.freedesktop.Hal.Device")
            for storage in self.storages:
                if properties["block.storage_device"] == storage["uid"]:
                    if properties["volume.label"] != "":
                        human_name = properties["volume.label"]
                    else:
                        human_name = storage["human_name"]
                    self.volumes.append({"uid" : volume_uid,
                                         "human_name" : human_name,
                                         "type": storage["type"],
                                         "mount_point": properties["volume.mount_point"],
                                         "is_mounted": properties["volume.is_mounted"]
                                         })
                    extras = {'real_uid': volume_uid}
                    self.dbus.add_signal_receiver(lambda *args: apply (self.__property_modified, args, extras),
                                                  "PropertyModified",
                                                  "org.freedesktop.Hal.Device",
                                                  "org.freedesktop.Hal",
                                                  volume_uid)

    def get_volumes(self):
        return self.volumes

    def get_volume(self, uid):
        volume = None
        for tmp_volume in self.volumes:
            if tmp_volume["uid"] == uid:
                volume = tmp_volume
                
        return volume

    def volume_unmount(self, uid):
        for volume in self.volumes:
            if volume["uid"] == uid:
                if (os.path.exists ("/usr/bin/gnome-mount") == True):
                    cmdline = "gnome-mount --hal-udi " + uid + " --unmount"
                elif (os.path.exists ("/usr/bin/pumount") == True):
                    cmdline = "pumount " + volume["mount_point"]
                os.system (cmdline)
                if volume["type"] == "cdrom":
                    cmdline = "eject " + volume["mount_point"]
                    os.system(cmdline)

                #device_dbus_obj = self.dbus.get_object("org.freedesktop.Hal", uid)
                #volume_device = dbus.Interface(device_dbus_obj, "org.freedesktop.Hal.Device")
                #volume_device.Unmount() # tampoco con Umount o UnMount
    
gobject.type_register(MSDeviceManager)
