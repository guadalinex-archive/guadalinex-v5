#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Pe�a <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica M�viles Espa�a S.A.U.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import MSD
import os

class MSDASendSMS(MSD.MSDAction):
    def __init__(self, act_manager, conf, conn_manager):        
        MSD.MSDAction.__init__(self, act_manager, conf, conn_manager)
        self.security_manager = MSD.MSDSecurityManager(conf)
        
    def get_default_conf (self):
        return { 'name' : "Mensajes",
                 'id' : 50,
                 'tooltip' : u"Env�a mensajes de texto y multimedia",
                 'url' : " http://www.mensajeriaweb.movistar.es",
                 'help_url' : "em_53.htm",
                 'connection' : None
                 }    
    def launch_action (self):
        ret = self.security_manager.launch_url(self.get_conf_key_value("url"))
        #ret = os.system("gnome-open %s " % self.get_conf_key_value("url"))
    
