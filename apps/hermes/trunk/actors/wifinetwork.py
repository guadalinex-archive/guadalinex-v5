# -*- coding: utf-8 -*-

import os.path

from deviceactor import DeviceActor
from gettext import gettext as _


NETWORKICON = os.path.abspath('actors/img/wifi.png')
NETWORKICONOFF = os.path.abspath('actors/img/wifioff.png')

class Actor(DeviceActor):

    __required__ = {
            'info.category': 'net.80211',
            'linux.subsystem': 'net',
            'info.product': 'WLAN Interface'
            }


    def on_added(self):
        interface = self.properties['net.interface']
        self.msg_render.show(_("Wifi"), 
                _("Wifi network interface conected: %s") % interface,
                NETWORKICON)


    def on_removed(self):
        interface = self.properties['net.interface']
        self.msg_render.show(_("Wifi"), 
                _("Wifi network interface disconected: %s") % interface,
                NETWORKICONOFF)
