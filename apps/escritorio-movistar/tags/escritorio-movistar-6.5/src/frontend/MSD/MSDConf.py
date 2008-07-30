#!/usr/bin/python
# -*- coding: utf-8 -*-
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

import MSD
from MSD.MSDPPPvariables import *
import os
import pickle
import time
import copy
import gtk
import gnome.ui
import gnomevfs
import md5
import time

class MSDConf:
    def __init__(self):
        self.observers = []
        self.load_conf()
        self.iconTheme = gtk.icon_theme_get_default()
        self.thumbFactory = gnome.ui.ThumbnailFactory(gnome.ui.THUMBNAIL_SIZE_LARGE)
        self.icon_flags = gnome.ui.ICON_LOOKUP_FLAGS_NONE

    
    def add_observer(self,observer):
        if observer is not None:
            self.observers.append(observer)

    def remove_observer(self,observer):
        if observer in self.observers:
            self.observers.remove(observer)

    def __notify_observers(self):
        
        for ob in self.observers:
            try:
                ob.configuration_changed(self)
            except Exception ,msg:
                print "ERROR: notificando conf changed"

    # Version
    def get_version(self):
        return self.conf["version"]
                
    #Bookmarks conf
    def get_bookmarks_list(self):
        bookmarks_list = []
        for bookmark in self.conf["bookmarks"].keys():
            bookmarks_list.append([bookmark] + self.conf["bookmarks"][bookmark])
        return bookmarks_list

    def get_bookmark_info_list(self, bookmark):
        return self.conf["bookmarks"][bookmark]

    def get_bookmark_url (self, name):
        print self.conf["bookmarks"].keys()
        return self.conf["bookmarks"][name][0]
    
    def set_bookmark_url (self, name, url):
        self.conf["bookmarks"][name][0] = url

    def get_bookmark_timestamp (self, name):
        return self.conf["bookmarks"][name][3]

    def get_bookmark_connection (self, name):
        return self.conf["bookmarks"][name][1]

    def set_bookmark_connection (self, name, connection_name):
        self.conf["bookmarks"][name][1] = connection_name

    def get_bookmark_icon(self, name):
        icon_type = self.conf["bookmarks"][name][2]
        if icon_type == None:
            fav_icon = gtk.Image()
            fav_icon.set_from_file (MSD.icons_files_dir + "favoritos_movistar.png")
            pixbuf = fav_icon.get_pixbuf()
        else:
            try:               
                pixbuf = self.iconTheme.load_icon(icon_type, 16, 0)
            except:
                try:
                    pixbuf = self.iconTheme.load_icon("text-x-generic", 16, 0)
                except:
                    print "Pixbuf error"
                    return None
        return pixbuf
    
    def add_bookmark (self, name, url, connection_name, old_name=None):
        if url.startswith("file://") == True:
            mime = gnomevfs.get_mime_type(url)
            try:
                info = gnomevfs.get_file_info(url)
                icon, result = gnome.ui.icon_lookup(self.iconTheme, self.thumbFactory,
                                                    url, "", self.icon_flags, mime, info)
            except:
                icon = "text-x-generic"
        else:
            if "movistar.es" in url :
                icon = None
            elif "telefonica" in url :
                icon = None
            else:
                icon = "stock_internet"
            
        if old_name == None:
            btime = time.time()
            self.conf["bookmarks"][name]=[url, connection_name, icon, btime]
        else:
            tmp = [url, connection_name, icon, self.get_bookmark_timestamp(old_name)]
            self.conf["bookmarks"].pop(old_name)
            self.conf["bookmarks"][name] = tmp
    
    def del_bookmark (self, name):
        try:
            self.conf["bookmarks"].pop(name)
        except KeyError :
            return False

    def exists_bookmark (self, name):
        return self.conf["bookmarks"].has_key(name)

    
    #Connections conf

    def get_connection_names_list(self):
        return self.conf["connections"].keys()

    def get_connection_info_dict (self, name):
        if self.conf["connections"].has_key(name):
            return self.conf["connections"][name]
        else:
            return None

    def get_default_connection_name (self):
        for name in self.conf["connections"].keys():
            if self.conf["connections"][name]["default"] == True :
                return name
    
    def set_default_connection_name (self, name):
        old_default_name = self.get_default_connection_name()
        self.conf["connections"][old_default_name]["default"] = False
        self.conf["connections"][name]["default"] = True
        
    def exists_connection (self, name):
        return self.conf["connections"].has_key(name)

    def add_connection (self, name, user, passwd,
                        ask_password = False,
                        profile_by_default = True,
                        profile_name = None,
                        auto_dns = True,
                        primary_dns = None,
                        secondary_dns = None,
                        domains = None,
                        proxy = False,
                        proxy_ip = None,
                        proxy_port = None,
                        default = False,
                        old_name=None):
        if old_name == None:
            self.conf["connections"][name] = { "user" : user,
                                               "pass" : passwd,
                                               "ask_password" : ask_password,
                                               "profile_by_default" : profile_by_default,
                                               "profile_name" :profile_name ,
                                               "auto_dns" : auto_dns,
                                               "primary_dns" : primary_dns,
                                               "secondary_dns" : secondary_dns,
                                               "domains" : domains,
                                               "proxy" : proxy,
                                               "proxy_ip" : proxy_ip,
                                               "proxy_port" : proxy_port,
                                               "default" : default
                                               }
        else:
            tmp = { "user" : user,
                    "pass" : passwd,
                    "ask_password" : ask_password,
                    "profile_by_default" : profile_by_default,
                    "profile_name" :profile_name ,
                    "auto_dns" : auto_dns,
                    "primary_dns" : primary_dns,
                    "secondary_dns" : secondary_dns,
                    "domains" : domains,
                    "proxy" : proxy,
                    "proxy_ip" : proxy_ip,
                    "proxy_port" : proxy_port,
                    "default" : default
                    }
            self.conf["connections"].pop(old_name)
            self.conf["connections"][name] = tmp
            
            for x in self.conf["bookmarks"].keys():
                if self.conf["bookmarks"][x][1] == old_name :
                    self.conf["bookmarks"][x][1] = name

            for x in self.conf["actions"].keys():
                if self.conf["actions"][x]["connection"] == old_name :
                    self.conf["actions"][x]["connection"] = name

        
    def del_connection (self, conn_name):
        if self.conf["connections"][conn_name]["default"] == True:
            self.set_default_connection_name("movistar Internet")
        
        self.conf["connections"].pop(conn_name)
        
        for bookmark in self.conf["bookmarks"].keys():
            if conn_name == self.get_bookmark_connection(bookmark) :
                self.set_bookmark_connection(bookmark, None)

        for action in self.conf["actions"].keys():
            if conn_name == self.conf["actions"][action]["connection"] :
                self.conf["actions"][action]["connection"] = None

    def get_connection_params(self, conn_name):
        #FIXME falta acabar la conf de los devices
        #Falta la configuracion de los proxy si estan
        #Y una clave de control de hardwre
        if self.exists_connection (conn_name) == True:
            connection = self.get_connection_info_dict(conn_name)
            device_number = self.get_device_selected()            
            params = {}
            params[PPP_PARAM_LOGIN_KEY] = connection["user"]
            params[PPP_PARAM_PASSWORD_KEY] = connection["pass"]

            if connection["profile_by_default"] == False and connection["profile_name"] != None:
                params[PPP_PARAM_APN_KEY] = connection["profile_name"]
            else:
                 params[PPP_PARAM_APN_KEY] = ""

            if connection["proxy"] == False:
                params[PPP_PARAM_USE_PROXY_KEY]="NO"
                params[PPP_PARAM_PROXY_ADDRESS_KEY]=""
                params[PPP_PARAM_PROXY_PORT_KEY]=""
            else :
                params[PPP_PARAM_USE_PROXY_KEY]="YES"
                params[PPP_PARAM_PROXY_ADDRESS_KEY] = connection["proxy_ip"]
                params[PPP_PARAM_PROXY_PORT_KEY] = connection["proxy_port"]

            if connection["auto_dns"] == True:
                params[PPP_PARAM_AUTO_DNS_KEY] ="YES"
                params[PPP_PARAM_PRIMARY_DNS_KEY] =""
                params[PPP_PARAM_SECUNDARY_DNS_KEY]=""
            else:
                params[PPP_PARAM_AUTO_DNS_KEY] ="NO"
                params[PPP_PARAM_PRIMARY_DNS_KEY] = connection["primary_dns"]
                params[PPP_PARAM_SECUNDARY_DNS_KEY]= connection["secondary_dns"]

            if connection["domains"] == '' or connection["domains"] == None:
                params[PPP_PARAM_DNS_SUFFIX_KEY]= ''
            else:
                params[PPP_PARAM_DNS_SUFFIX_KEY]= connection["domains"]
            
            # TODO cogerlo de la conexion
            params[PPP_PARAM_CYPHER_PASSWORD_KEY] = "NO"
            
            #propiedades del dispositivo
               
##             params[PPP_PARAM_DEVICE_PORT_KEY] = self.get_device_path_of_selected_device()
##             params[PPP_PARAM_DEVICE_SPEED_KEY]= self.get_speed_of_selected_device()
##             params[PPP_PARAM_COMPRESSION_KEY] = self.get_compression_flag_of_selected_device()
##             params[PPP_PARAM_FLOW_CONTROL_KEY] = self.get_flow_control_of_selected_device()
##             params[PPP_PARAM_ERROR_CONTROL_KEY] = self.get_error_control_flag_of_selected_device()

            return params
        else:
            return None
    
        
    #Security conf

    def get_auth_activate(self):
        return self.conf["security"]["auth_on"]

    def set_auth_activate(self, value):
        if value == True:
            self.conf["security"]["auth_on"] = True
        else:
            self.conf["security"]["auth_on"] = False

    def get_celular_info(self):
        return [self.conf["security"]["celular_number"],
                self.conf["security"]["celular_password"]]

    def set_celular_info(self, number, password):
        self.conf["security"]["celular_number"] = number
        self.conf["security"]["celular_password"] = password

    def get_ask_password_activate(self):
        return self.conf["security"]["ask_password"]

    def set_ask_password_activate(self, value):
        if value == True:
            self.conf["security"]["ask_password"] = True
        else:
            self.conf["security"]["ask_password"] = False

    # Devices conf
    def get_device_selected(self):
        return self.conf["devices"]["selected"]

        
    def get_device_selected_name(self):
        return self.__device_get_device_name(self.conf["devices"]["selected"])
        
    def set_device_selected(self, device_number):
        self.conf["devices"]["selected"] = device_number

    def __device_get_device_name(self, device_number):
        if (device_number == 0):
            device_name = "Bluetooth"
        elif (device_number == 1):
            device_name = "Infrarrojos"
        elif (device_number == 2):
            device_name = "USB"
        elif (device_number == 3):
            device_name = "Puerto serie"
        elif (device_number == 4):
            device_name = "Option"
        else:
            device_name = "NingÃºn dispositivo seleccionado"

        return device_name

    def set_device_config_dev(self, device_number, dev):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        self.conf["devices"][device_name]["dev"] = dev

    def get_device_config_dev(self, device_number):
        device_name = self.__device_get_device_name (device_number)
        print "MSDCONF : method -> get_device_config_dev () device_name = %s" % device_name
        if (device_name == ""):
            return
        return self.conf["devices"][device_name]["dev"]

    def set_device_config_velocity(self, device_number, velocity):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        self.conf["devices"][device_name]["velocity"] = velocity

    def get_device_config_velocity(self, device_number):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        return self.conf["devices"][device_name]["velocity"]

    def set_device_config_hfc(self, device_number, hfc):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        self.conf["devices"][device_name]["hardware_flow_control"] = hfc

    def get_device_config_hfc(self, device_number):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        return self.conf["devices"][device_name]["hardware_flow_control"]

    def set_device_config_hec(self, device_number, hec):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        self.conf["devices"][device_name]["hardware_error_control"] = hec

    def get_device_config_hec(self, device_number):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        return self.conf["devices"][device_name]["hardware_error_control"]

    def set_device_config_hcss(self, device_number, hcss):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        self.conf["devices"][device_name]["hardware_compress"] = hcss

    def get_device_config_hcss(self, device_number):
        device_name = self.__device_get_device_name (device_number)
        if (device_name == ""):
            return
        return self.conf["devices"][device_name]["hardware_compress"]

    def get_old_other_dev_selected(self):
        return self.conf["devices"]["old_other_dev_selected"]

    def set_old_other_dev_selected(self, num):
        self.conf["devices"]["old_other_dev_selected"] = num

    #metodos de conveniencia para el acceso a dispositivos
    def get_property_of_selected_device(self,property_name):
        device_number = self.get_device_selected() 
        device_name = self.__device_get_device_name (device_number)
        print "Obtenido propiedad %s del dispositivo %s" % (property_name,device_name)
        return self.conf["devices"][device_name][property_name]

    
    def get_device_path_of_selected_device(self):
        value = self.get_property_of_selected_device("dev")
        return value
    
    def get_speed_of_selected_device(self):
         value = self.get_property_of_selected_device("velocity")
         return str(value)
     
    def get_compression_flag_of_selected_device(self):
        value = self.get_property_of_selected_device("hardware_compress")
        if value:
            return "YES"
        else:
            return "NO"
        
    def get_flow_control_of_selected_device(self):
        value = self.get_property_of_selected_device("hardware_flow_control")
        if value:
            return "YES"
        else:
            return "NO"
    def get_error_control_flag_of_selected_device(self):
        value = self.get_property_of_selected_device("hardware_error_control")
        if value:
            return "YES"
        else:
            return "NO"
    

    #Actions Conf

    def exists_action_conf (self, codename):
        return self.conf["actions"].has_key(codename)

    def set_default_action_conf (self, codename, dict):
        self.conf["actions"][codename] = dict

    def set_action_key_value (self, codename, key, value):
        self.conf["actions"][codename][key] = value

    def get_action_key_value (self, codename, key):
        return self.conf["actions"][codename][key]

    def get_hide_uninstalled_services(self):
        return self.conf["actions_general"]["hide_uninstalled"]

    def set_hide_uninstalled_services(self, hide):
        self.conf["actions_general"]["hide_uninstalled"] = hide

    def get_uninstalled_services_list(self):
        list = []
        for x in self.conf["actions"].keys() :
            if self.get_action_key_value(x, "installed") == False:
                list.append(x)
        return list

    def get_actions_order(self):
        return self.conf["actions_general"]["actions_order"]

    def get_original_actions_order(self):
        return self.conf["actions_general"]["original_actions_order"]

    def set_original_actions_order(self):
        self.conf["actions_general"]["actions_order"] = copy.copy(self.conf["actions_general"]["original_actions_order"])
        self.save_conf()

    #UI General key value

    def get_ui_general_key_value(self, key):
        return self.conf["ui_general"][key]

    def set_ui_general_key_value(self, key, value):
        self.conf["ui_general"][key] = value

    # Consumo y velocidad acumulada
    
    def get_acumulated_sent_traffic(self, roaming):
        if roaming == False:
            return self.conf['expenses']['sent']
        else:
            return self.conf['expenses']['sent_roaming']

    def get_acumulated_received_traffic(self, roaming):
        if roaming == False:
            return self.conf['expenses']['recived']
        else:
            return self.conf['expenses']['recived_roaming']

    def get_acumulated_traffic(self, roaming):
        if roaming == False:
            return self.conf['expenses']['traffic']
        else:
            return self.conf['expenses']['traffic_roaming']

    def get_acumulated_time(self, roaming):
        if roaming == False:
            return self.conf['expenses']['time']
        else:
            return self.conf['expenses']['time_roaming']
        
    def get_max_upload_speed(self):
        return self.conf['expenses']['max_upload_speed']

    def get_max_download_speed(self):
        return self.conf['expenses']['max_download_speed']

    def set_acumulated_sent_traffic(self,value, roaming):
        if roaming == False:
            self.conf['expenses']['sent'] = value
        else:
            self.conf['expenses']['sent_roaming'] = value
        
    def set_acumulated_received_traffic(self,value, roaming):
        if roaming == False:
            self.conf['expenses']['recived'] = value
        else:
            self.conf['expenses']['recived_roaming'] = value

    def set_acumulated_traffic(self,value, roaming):
        if roaming == False:
            self.conf['expenses']['traffic'] = value
        else:
            self.conf['expenses']['traffic_roaming'] = value

    def set_acumulated_time(self,value, roaming):
        if roaming == False:
            self.conf['expenses']['time'] = value
        else:
            self.conf['expenses']['time_roaming'] = value

    def set_max_upload_speed(self,value):
        self.conf['expenses']['max_upload_speed'] = value

    def set_max_download_speed(self,value):
        self.conf['expenses']['max_download_speed'] = value
        
    def reset_acumulated(self):
        self.conf['expenses']['sent'] = 0
        self.conf['expenses']['recived'] = 0
        self.conf['expenses']['traffic'] = 0
        self.conf['expenses']['time'] = 0
        
        self.conf['expenses']['sent_roaming'] = 0
        self.conf['expenses']['recived_roaming'] = 0
        self.conf['expenses']['traffic_roaming'] = 0
        self.conf['expenses']['time_roaming'] = 0
        
        self.conf['expenses']['max_upload_speed'] = 0
        self.conf['expenses']['max_download_speed'] = 0
        self.conf['expenses']['last_resumen'] = time.localtime()
        self.save_conf()

    def get_show_mb_warning(self, roaming):
        if roaming == False:
            return self.conf['expenses']['show_mb_warning']
        else:
            return self.conf['expenses']['show_mb_warning_roaming']

    def set_show_mb_warning(self, value, roaming):
        if roaming == False:
            self.conf['expenses']['show_mb_warning'] = value
        else:
            self.conf['expenses']['show_mb_warning_roaming'] = value

    def get_show_percent_warning(self):
        return self.conf['expenses']['show_percent_warning']

    def set_show_percent_warning(self, value):
        self.conf['expenses']['show_percent_warning'] = value

    def get_warning_mb_combo(self):
        return self.conf['expenses']['warning_mb_combo']

    def set_warning_mb_combo(self, value):
        self.conf['expenses']['warning_mb_combo'] = value

    def get_warning_mb(self, roaming):
        if roaming == False:
            return self.conf['expenses']['warning_mb']
        else:
            return self.conf['expenses']['warning_mb_roaming']

    def set_warning_mb(self, value, roaming):
        if roaming == False:
            self.conf['expenses']['warning_mb'] = value
        else:
            self.conf['expenses']['warning_mb_roaming'] = value

    def get_warning_percent(self):
        return self.conf['expenses']['warning_percent']

    def set_warning_percent(self, value):
        self.conf['expenses']['warning_percent'] = value

    
    def get_user_id(self):
        user_id = self.conf['user_id']
        if user_id is None:
            user_id = md5.new("EMLIN-%s" % str(time.time())).hexdigest()        
            self.conf['user_id'] = user_id
            self.save_conf()
            
        return user_id

    def get_clean_resumen(self):
        return self.conf['expenses']['clean_resumen']

    def set_clean_resumen(self, value):
        self.conf['expenses']['clean_resumen'] = value
        self.save_conf()

    def get_last_resumen(self):
        if self.conf['expenses']['last_resumen'] == None:
            self.conf['expenses']['last_resumen'] = time.localtime()
            self.save_conf()
        
        return self.conf['expenses']['last_resumen']

    def set_last_resumen(self, value):    
        self.conf['expenses']['last_resumen'] = value
        self.save_conf()

    # RSS Feed
    def get_updater_feed_url(self):
        return self.conf['updater_feed_url']

    def get_updater_feed_date(self):
        return self.conf['updater_feed_date']

    def set_updater_feed_date(self, date):
        self.conf['updater_feed_date'] = date

    def get_release_date(self):
        return self.conf['release_date']
    
    #SyncML:
    def get_country_code(self):
    	return self.conf['country_code']
    	
    def get_syncml_server(self):
    	return self.conf['syncml']['server']
    	
    def get_syncml_service(self):
    	return self.conf['syncml']['service']
    	
    def get_syncml_resource(self):
    	return self.conf['syncml']['resource']

    #rss on connect

    def get_rss_on_connect(self):
        return self.conf['rss_on_connect']

    def set_rss_on_connect(self, value):
        self.conf['rss_on_connect'] = value
        self.save_conf()
    	
    #Load/Save Methods
    
    def load_conf(self):
        if os.path.isfile(MSD.conf_file) :
            fd = open(MSD.conf_file, "r")
            try:
                self.conf = pickle.loads(fd.read())
            except:
                print "----> Fichero de configuracion corrupto. Restaurando predefinido"
                if os.path.exists(MSD.conf_file + ".bak") :
                    print "-----> restaurando ichero backup"
                    fd = open(MSD.conf_file + ".bak" , "r")
                    try:
                        self.conf = pickle.loads(fd.read())
                    except:
                        print "-----> No pudo ser restaurado el backup, vuelta a configuracion original"
                        new_conf = self.__default_conf()
                        self.conf = new_conf
                        self.save_conf()
                else:
                    print "-----> No pudo ser restaurado el backup, vuelta a configuracion original"
                    new_conf = self.__default_conf()
                    self.conf = new_conf
                    self.save_conf()
                
            if not self.conf.has_key("version"):
                new_conf = self.__default_conf()
                new_conf["bookmarks"] = self.conf["bookmarks"]
                self.conf = new_conf
                self.save_conf()
            else:
                if self.conf["version"] == "6.0" :
                    
                    new_conf = self.__default_conf()
                    new_conf["bookmarks"] = self.conf["bookmarks"]
                    new_conf["connections"] = self.conf["connections"]
                    new_conf["connections"]["movistar Internet"] = { "user" : "MOVISTAR",
                                                                     "pass" : "MOVISTAR",
                                                                     "ask_password" : False,
                                                                     "profile_by_default" : True,
                                                                     "profile_name" : None,
                                                                     "auto_dns" : False,
                                                                     "primary_dns" : "194.179.1.100",
                                                                     "secondary_dns" : "194.179.1.101",
                                                                     "domains" : None,
                                                                     "proxy" : False,
                                                                     "proxy_ip" : None,
                                                                     "proxy_port" : "0",
                                                                     "default" : False
                                                                     }
                    new_conf["connections"]["movistar Internet directo"] = { "user" : "MOVISTAR_DIRECTO",
                                                                             "pass" : "MOVISTAR_DIRECTO",
                                                                             "ask_password" : False,
                                                                             "profile_by_default" : True,
                                                                             "profile_name" : None,
                                                                             "auto_dns" : False,
                                                                             "primary_dns" : "194.179.1.100",
                                                                             "secondary_dns" : "194.179.1.101",
                                                                             "domains" : None,
                                                                             "proxy" : False,
                                                                             "proxy_ip" : None,
                                                                             "proxy_port" : "0",
                                                                             "default" : False
                                                                             }
                    self.set_default_connection_name("movistar Internet")
                    self.conf = new_conf
                    self.conf["connections"].pop("Internet GPRS/3G")
                    self.save_conf()

            self.conf["version"] = "6.5"
            self.save_conf()
            fd.close()
                
        else:
            self.conf = self.__default_conf()
            print self.conf
            self.save_conf ()

    def save_conf(self):
        if self.conf != None:
            if os.path.isdir(os.path.dirname(MSD.conf_file)) == False:
                os.mkdir(os.path.dirname(MSD.conf_file))
            os.system("cp %s %s" % (MSD.conf_file, MSD.conf_file + ".bak"))
            fd = open(MSD.conf_file, "w")
            fd.write(pickle.dumps(self.conf))
            fd.close()
        self.__notify_observers()
        
    def __default_conf (self):
        def_conf = {'version' : "6.5",
                    'release_date' : "Sun, 9 Dec 2007 12:30:22 GMT",
                    'bookmarks' : {_('Canal Cliente') : ['http://www.canalcliente.movistar.es', None, None, 1],
                                   _('Yavoy') : ['http://www.yavoy.movistar.es', None, None, 2],
                                   _('Cobertura GPRS\\3G\\3.5G') : ['http://www.cobertura.movistar.es', None, None, 3],
                                   _('Cobertura Zona ADSL\\WiFi de Telefónica') : ['http://www.telefonicaonline.com/zonawifi', None, None, 4]
                               },
                    'connections' : {'movistar Internet' : { "user" : "MOVISTAR",
                                                             "pass" : "MOVISTAR",
                                                             "ask_password" : False,
                                                             "profile_by_default" : True,
                                                             "profile_name" : None,
                                                             "auto_dns" : False,
                                                             "primary_dns" : "194.179.1.100",
                                                             "secondary_dns" : "194.179.1.101",
                                                             "domains" : None,
                                                             "proxy" : False,
                                                             "proxy_ip" : None,
                                                             "proxy_port" : "0",
                                                             "default" : True
                                                            },
                                     'movistar Internet directo' : { "user" : "MOVISTAR_DIRECTO",
                                                            "pass" : "MOVISTAR_DIRECTO",
                                                            "ask_password" : False,
                                                            "profile_by_default" : True,
                                                            "profile_name" : None,
                                                            "auto_dns" : False,
                                                            "primary_dns" : "194.179.1.100",
                                                            "secondary_dns" : "194.179.1.101",
                                                            "domains" : None,
                                                            "proxy" : False,
                                                            "proxy_ip" : None,
                                                            "proxy_port" : "0",
                                                            "default" : False
                                                            },
                                     },
                    'connections_general' : {'ask_before_connect_to_action' : True,
                                             'ask_before_change_connection' : True ,
                                             'ask_before_connect' : True},
                    'user_id' : None,
                    'security' : { 'auth_on' : True,
                                   'celular_number' : None,
                                   'celular_password' : None,
                                   'ask_password' : False
                                   },
                    'devices' : { 'Bluetooth' : { 'dev': "/dev/rfcomm0",
                                                  'velocity': 57600,
                                                  'hardware_flow_control' : False,
                                                  'hardware_error_control' : False,
                                                  'hardware_compress' : False},
                                  'Infrarrojos' : {'dev': "/dev/ircomm0",
                                                  'velocity': 57600,
                                                  'hardware_flow_control' : False,
                                                  'hardware_error_control' : False,
                                                  'hardware_compress' : False},
                                  'USB': {'dev': "/dev/ttyACM0",
                                          'velocity': 57600,
                                          'hardware_flow_control' : False,
                                          'hardware_error_control' : False,
                                          'hardware_compress' : False},
                                  'Puerto serie' : {'dev': "/dev/ttyS0",
                                                    'velocity': 57600,
                                                    'hardware_flow_control' : False,
                                                    'hardware_error_control' : False,
                                                    'hardware_compress' : False},
                                  'Option' : {'model' :"Option",
                                              'dev': "/dev/ttyUSB0",
                                              'velocity': 57600,
                                              'hardware_flow_control' : False,
                                              'hardware_error_control' : False,
                                              'hardware_compress' : False},
                                  'selected' : 4,
                                  'old_other_dev_selected' : 0
                                  },
                    'expenses' : { 'sent':0,
                                   'sent_roaming':0,
                                   'recived':0,
                                   'recived_roaming':0,
                                   'traffic': 0,
                                   'traffic_roaming' : 0, 
                                   'time':0,
                                   'time_roaming':0,
                                   'max_upload_speed':0,
                                   'max_download_speed':0,
                                   'show_mb_warning': False,
                                   'show_mb_warning_roaming':False,
                                   'show_percent_warning': False,
                                   'warning_mb': 200,
                                   'warning_mb_roaming': 10,
                                   'warning_mb_combo': 0,
                                   'warning_percent': 80,
                                   'clean_resumen' : True,
                                   'last_resumen' : None
                                  },
                    
                    'actions' : {},
                    'actions_general' : {'hide_uninstalled' : True,
                                         'actions_order' : ["MSDAInternet",
                                                            "MSDASendSMS",
                                                            "MSDASendMMS",
                                                            "MSDAMovilidad",
                                                            "MSDAIntranet"],
                                         'original_actions_order' : ["MSDAInternet",
                                                                     "MSDASendSMS",
                                                                     "MSDASendMMS",
                                                                     "MSDAMovilidad",
                                                                     "MSDAIntranet"]
                                         },
                    'ui_general' : {'main_expander_on' : True,
                                    'wellcome_warning_show' : False,
                                    'show_new_bookmark_confirmation': True,
                                    'systray_showing_mw': True},
                    'updater_feed_url' : "http://www.movistar.es/empresas/servicios/descargaaplicaciones/linux/rss.xml",
#                    'updater_feed_url': "http://195.235.93.150/rss.xml",
                    'updater_feed_date' : "",
                    'country_code' : "+34",
                    'syncml' : {'server' : 'sync.movistar.es',
                                'service' : 'syncml',
                                'resource' : './pab'
                                },
                    'rss_on_connect' : True
                    }
        
        
        return def_conf

    
