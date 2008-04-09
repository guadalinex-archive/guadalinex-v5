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


class MSDASendMMS(MSD.MSDAction):
    def __init__(self, act_manager, conf, conn_manager):
        print "Init MSDASendMMS"
        MSD.MSDAction.__init__(self, act_manager, conf, conn_manager)
        self.security_manager = MSD.MSDSecurityManager(conf)
        self.webmail_client_radiobutton = self.get_prefs_widget("webmail_client_radiobutton")
        self.mail_client_radio_button = self.get_prefs_widget("mail_client_radio_button")
        self.mail_client_filechooser = self.get_prefs_widget("mail_client_filechooser")
        #self.mail_client_filechooser.dialog.set_icon(self.get_dialog_icon())
        #print self.mail_client_filechooser.dialog

        if self.get_conf_key_value("mail_client_uri") != None:
            print "init MSDASendMMS -> %s" % self.get_conf_key_value("mail_client_uri")
            self.mail_client_filechooser.set_uri(self.get_conf_key_value("mail_client_uri"))
        else:
            print "init MSDASendMMS -> %s" % self.get_conf_key_value("mail_client_uri")
        self.mail_client_filechooser.show()
        
        if self.get_conf_key_value("webmail_by_default") == True:
            self.webmail_client_radiobutton.set_active(True)
            self.mail_client_filechooser.set_sensitive(False)
        else:
            self.mail_client_radio_button.set_active(True)
            self.mail_client_filechooser.set_sensitive(True)

        self.webmail_client_radiobutton.connect("toggled", self.__webmail_client_radio_button_cb, None)
        self.mail_client_filechooser.connect("selection-changed", self.__mail_client_filechooser_cb, None)

    def __webmail_client_radio_button_cb(self, widget, data):
        if self.webmail_client_radiobutton.get_active() == True:
            self.mail_client_filechooser.set_sensitive(False)
            self.set_conf_key_value("webmail_by_default", True)
        else:
            self.mail_client_filechooser.set_sensitive(True)
            self.set_conf_key_value("webmail_by_default", False)
    
    def __mail_client_filechooser_cb(self, widget, data):
        if self.mail_client_filechooser.get_uri() == None or self.mail_client_filechooser.get_uri() == "":
            self.set_conf_key_value("mail_client_uri", None)
        else:
            self.set_conf_key_value("mail_client_uri", self.mail_client_filechooser.get_uri())
    
    def get_default_conf (self):
        return {'name' : u"Correo M�vil",
                'id' : 30,
                'tooltip' : u"Accede a tu cuenta del servicio Correo M�vil",
                'url' : "http://www.correomovil.movistar.es",
                'webmail_by_default' : True,
                'mail_client_uri' : None,
                'help_url' : 'em_52.htm',
                'connection' : None
                 } 

    def launch_action(self):
        import os
        if self.get_conf_key_value("webmail_by_default") == True:
            self.security_manager.launch_url(self.get_conf_key_value("url"))
            #os.system("gnome-open %s " % self.get_conf_key_value("url"))
        else:
            program = self.get_conf_key_value("mail_client_uri")
            if program == None:
                return
            program = program.replace("file://","")            
            print program
            os.spawnle(os.P_NOWAIT, "%s" % program, "", os.environ)
