# -*- coding: utf-8 -*-

import os.path

from deviceactor import DeviceActor
from gettext import gettext as _

NETWORKICON = os.path.abspath('actors/img/network.png')
NETWORKICONOFF = os.path.abspath('actors/img/networkoff.png')

class Actor (DeviceActor):

    __required__ = {
    "linux.subsystem":"net"
    }


    def on_added(self):
        interface = self.properties['net.interface']
        self.msg_render.show(_("Network"), 
                _("Connected network interface %s") % interface,
                NETWORKICON)


    def on_removed(self):
        interface = self.properties['net.interface']
        self.msg_render.show(_("Network"), 
                _("Disconnected network interface %s") % interface,
                NETWORKICONOFF)

