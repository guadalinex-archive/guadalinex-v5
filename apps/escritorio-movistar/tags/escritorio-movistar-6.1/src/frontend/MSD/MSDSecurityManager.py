#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica Móviles España S.A.U.
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

import os
import gtk
import MSD
import tempfile

TEMPLATE_DATA ="""<html>
<body>
<style type="text/css">
input.flat{
	color:#333;
  	font-family:'trebuchet ms',helvetica,sans-serif;
  	font-size:84%%;
  	font-weight:bold;
  	border:1px solid;
  	border-top-color:#999;
  	border-left-color:#999;
  	border-right-color:#666;
  	border-bottom-color:#666;
}
input.invisible{
  background-color:transparent;
  border:0px;
  display:none;
}


</style>

<p align="center">
<form id="frm" action="%(url)s" method="POST">
	<input type="hidden" name="TM_LOGIN" value="%(login)s">
	<input type="hidden" name="TM_PASSWORD"  value="%(password)s">
	<input type="hidden" name="TM_ACTION"  value="LOGIN">
	<input type="hidden" name="URL"  value="%(url)s">
    <input class="invisible" id="sub" type="submit" class="flat" value="">
</form>
</p>
<script language="JavaScript"><!--
	document.getElementById("sub").value="";
	document.getElementById("sub").className="invisible";
	document.getElementById("frm").submit();
//-->
</script>
</body>
</html>"""
        
class Singleton(object):
        _instance = None
        def __new__(cls, *args, **kwargs):
            if not cls._instance:
                print "Creando nueva instancia"
                cls._instance = super(Singleton, cls).__new__(cls, *args, **kwargs)
            return cls._instance

        
class MSDSecurityManager(Singleton):
    """
    Esta clase encapsula la gestion de los seervicios seguros 
    """

    def __init__(self,conf):
        print "Sec mananager init"
        if not  hasattr(self,"conf"):
            self.conf = conf
            self.xml = gtk.glade.XML(MSD.glade_files_dir + "security.glade")
            self.security_dialog = self.xml.get_widget("security_dialog")
            self.phone_entry =  self.xml.get_widget("phone_entry")
            self.password_entry = self.xml.get_widget("password_entry")
            self.remember_data_checkbutton = self.xml.get_widget("remember_data_checkbutton")
	    self.help_button =  self.xml.get_widget("help_button")
	    self.help_button.connect("clicked",self.__help_button_cb)
            self.ok_button = self.xml.get_widget("ok_button")
            self.phone_entry.connect("changed", self.__security_dialog_entry_changed, None)
            self.password_entry.connect("changed", self.__security_dialog_entry_changed, None)
            self.security_dialog.set_icon_from_file(MSD.icons_files_dir + "security_24x24.png")
        else:
            print "Ignorando parametros el objeto ya esta construido"

        
    def launch_url(self,url):
        """
        lanza la url  de mananera compatible con la seguirdad unificada de TME.
        Si la seguridad NO  esta activada la url se lana de la manera normal
        """
        if self.conf.get_auth_activate():
            self.launch_secure_url(url)
        else:
            print "lanzando URL NO segura"
            return  os.system("gnome-open %s " % url)

    def launch_secure_url(self,url):
	print "LANZANDO URL SEGURA %s" % url
	current_login ,current_password =  self.conf.get_celular_info()

        
	if  self.__should_ask_password() :
		response ,current_login , current_password =  self.__run_security_dialog()
		if response != gtk.RESPONSE_OK:
			    return 

	template_dict = {"url" : url,
                         "login": current_login,
                         "password": current_password}
        file_name =self.__create_temporal_file(template_dict)
        if file_name is not None:
		ret = os.popen("gconftool-2 -g /desktop/gnome/url-handlers/http/command")
		url_cmd = ret.readline().split()
		if len(url_cmd) > 0 :
			os.system("%s %s &" % (url_cmd[0], file_name))
        else:
            print "ERROR creando fichero temporal"

    def __help_button_cb(self,widget):
	    dir_name =os.path.dirname(MSD.help_uri)
    	    help_file = os.path.join(dir_name,"em_62.htm#Datosusuario")
	    ret = os.popen("gconftool-2 -g /desktop/gnome/url-handlers/http/command")
	    url_cmd = ret.readline().split()
	    if len(url_cmd) > 0 :
		    os.system("%s 'file://%s' &" % (url_cmd[0], help_file))
	    
    def __create_temporal_file(self,template_dict):
        try:
            os.system("rm -rf /tmp/em-*")
            # creo el fichero temporal
            fd ,file_name = tempfile.mkstemp("","em-")
            f = os.fdopen(fd,"w")
            print template_dict
            f.write(TEMPLATE_DATA % template_dict)
            f.close()
            return file_name
        except Exception ,msg:
            print "error creando temporal file url %s " % msg
            return None
        
        

    def __should_ask_password(self):
        if self.conf.get_ask_password_activate():
            return True
        login ,password =  self.conf.get_celular_info()

        if login is None or password is None:
            return True

        if len(login) == 0 or len(password) == 0:
            return True
        
        return False

    def __run_security_dialog(self):

        #inicializo los campos
        current_login ,current_password =  self.conf.get_celular_info()
        if current_password is None :
            current_password = ""
        if current_login is None:
            current_login = ""
        
        self.phone_entry.set_text(current_login)

        if  self.conf.get_ask_password_activate():
            self.password_entry.set_text("")
            self.remember_data_checkbutton.set_active(False)
        else:
            self.password_entry.set_text(current_password)
            self.remember_data_checkbutton.set_active(True)
        self. __security_dialog_entry_changed(self.password_entry,None)

        while True:
            response = self.security_dialog.run()
            self.security_dialog.hide()        
            login =  self.phone_entry.get_text()
            password = self.password_entry.get_text()
            if response != gtk.RESPONSE_OK:
                break
            try:                
                int(login)
                break
            except Exception, msg:
                 dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
		 dlg.set_icon_from_file(MSD.icons_files_dir + "security_24x24.png")
                 dlg.set_markup(MSD.MSG_INVALID_PHONE_TITLE)
                 dlg.format_secondary_markup(MSD.MSG_INVALID_PHONE)
                 dlg.run()
                 dlg.destroy()
        
        if response == gtk.RESPONSE_OK and self.remember_data_checkbutton.get_active():
            #salvo los datos
            self.conf.set_celular_info(login,password)
            self.conf.set_ask_password_activate(False)
            self.conf.save_conf()
        
        return response,login,password

    def __security_dialog_entry_changed(self,widget,data):
        if len(self.password_entry.get_text()) < 1 or len(self.phone_entry.get_text()) < 1:
            self.ok_button.set_sensitive(False)
        else:
            self.ok_button.set_sensitive(True)
    
    
if __name__ == '__main__':
    a = MSDSecurityManager([1,2,3])
    b = MSDSecurityManager([4,5,6])
    print a
    print b
    print a.conf
    print b.conf
    
    

        
    
    
    
