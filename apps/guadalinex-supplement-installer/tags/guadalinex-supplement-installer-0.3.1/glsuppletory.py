#!/usr/bin/python
# -*- coding: utf-8 -*-
import logging
import os.path
import sys
sys.path.insert(0, '/usr/share/gsd')

import config
import appinstall
import gsdutils
from gsdutils import SupplementCustomizer
from utils.synaptic import Synaptic
from volume import Actor

GLVALIDLABELS = config.GLVALIDLABELS
#GAI (guadalinex-app-install) Packages
GAIPACKAGES = ['gnome-app-install']

#Supplement icon (relative to cdrom path)
RELATIVEICONPATH = '.icon.png'

class GlSuppletory(object):
    """
    This class encapsule a volume.Actor hack which can detect Guadalinex
    Suppletory Disks.
    """

    def __init__(self):
        self.logger = logging.getLogger()
        #Current volume.Actor hacked object
        self.volume_actor = None

    def hack(self, volume_actor):
        """
        <volume_actor> must be a volume.Actor object
        """

        self.volume_actor = volume_actor
        s = Synaptic()
        mountpoint = self.volume_actor.properties['volume.mount_point']

        def action_install_gai():
            s.install(GAIPACKAGES)
            self.show_supplement_info()

        def action_install_sup():
            os.system('gksudo guadalinex-app-install %s' %\
                    mountpoint)

        #Check for label and  README.diskdefines
        volumelabel = self.volume_actor.properties['volume.label']
        if self.__is_valid(volumelabel):
            s = Synaptic()
            actions = {}
            suppc = SupplementCustomizer(mountpoint)
            diskdefines = suppc.get_diskdefines()
            if diskdefines:
                #Check for required packages
                if s.check(GAIPACKAGES):
                    actions = {
                        "Instalar Suplemento": action_install_sup
                    }
                else:
                    actions = {
                        "Instalar herramientas para suplementos" : action_install_gai
                        }

                diskname = ''
                for key, value in diskdefines:
                    if key == 'DISKNAME':
                        diskname = value
                        break;

                message  = diskname
                summary = "Guadalinex Suplementos"
                iconpath = mountpoint + '/' + RELATIVEICONPATH
                if os.path.isfile(iconpath):
                    self.volume_actor.msg_render.show(summary, message, 
                            icon = iconpath, actions = actions)

                else:
                    self.volume_actor.msg_render.show_info(summary, message,
                            actions = actions)


    def show_supplement_info(self):
        ddpath = self.volume_actor.properties['volume.mount_point']
        #parser = DiskDefinesParser()


    def hack_volume_actor(self):

        def new_on_modified(volume_actor, prop_name):
            Actor.old_on_modified(volume_actor, prop_name)
            if prop_name == 'volume.is_mounted' and \
                    volume_actor.properties['volume.is_mounted']:
                self.hack(volume_actor)
        
        #Hacking volume.Actor class
        Actor.old_on_modified = Actor.on_modified
        Actor.on_modified = new_on_modified

    def __is_valid(self, label):
        """
        Check if <labes> is a valid label for Guadalinex cd
        """
        return config.is_valid_label(label)

gls = GlSuppletory()
gls.hack_volume_actor()
