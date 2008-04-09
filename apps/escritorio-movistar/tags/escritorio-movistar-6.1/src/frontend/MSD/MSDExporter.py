#!/usr/bin/python
# -*- coding: iso-8859-15 -*-
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
import os
import codecs

# extensiones .emsc .msgprs

class MSDExporter:
    def __init__(self, conf):
        self.conf = conf

    def save_connection_to_file(self, conn, file):
        dict = {}
        dict = self.__get_connection_params(dict, conn)
        self.__print_dict_to_file(dict, file)

    def save_bookmark_to_file(self, bookmark, file):
        dict = {}
        dict = self.__get_bookmark_params(dict, bookmark)
        self.__print_dict_to_file(dict, file)


    def __print_dict_to_file(self, dict, file="/tmp/prueba.emsc"):
        keys = dict.keys()
        keys.sort()
        fd = codecs.open(file, "w", "latin-1")

        print "---------------------Export dict-----------------------"
        print dict
        print repr(dict)
        print "-------------------------------------------------------"

        for key in keys:
            if key == "Fichero inicio":
                fd.write('%s = %s\n' % (key.decode("latin-1"), dict[key].decode("latin-1")))
            else:
                fd.write('%s = %s\n' % (key.decode("latin-1"), unicode(dict[key], "utf-8")))
        
        fd.close()
            

    def __get_bookmark_params(self, dict, bookmark):
        bookmark_info = self.conf.get_bookmark_info_list(bookmark)
        
        dict["Nombre servicio"] = bookmark

        #FIXME : Autenticacion unificada
        if bookmark_info[0].startswith("file://") == True:
            dict["Abrir fichero"] = "1"
            dict["Abrir url"] = "0"
            dict["Fichero inicio"] = bookmark_info[0]
            dict["Url inicio"] = ""
            dict["favorito.movistar"] = "0"
            dict["favorito.autenticacion_unificada"] = "0"
        else:
            if "movistar.es" in bookmark_info[0] :
                dict["favorito.movistar"] = "1"
            else:
                dict["favorito.movistar"] = "0"

            dict["Abrir fichero"] = "0"
            dict["Abrir url"] = "1"
            dict["Fichero inicio"] = ""
            dict["Url inicio"] = bookmark_info[0]
            dict["favorito.autenticacion_unificada"] = "0"

        if bookmark_info[1] != None and  bookmark_info[1] != "":
            return self.__get_connection_params(dict, bookmark_info[1])
        else:
            return self.__get_connection_params(dict, self.conf.get_default_connection_name())

    def __get_connection_params(self, dict, conn):
        conn_info = self.conf.get_connection_info_dict(conn)

        if conn_info["default"] == True:
            dict["Conexión por defecto"] = "1"
        else:
            dict["Conexión por defecto"] = "0"

        dict["Servicio"] = conn
        dict["Nombre usuario"] = conn_info["user"]

        if conn_info["profile_name"] != None and conn_info["profile_name"] != "" :
            dict["APN"] = conn_info["profile_name"]
        else:
            dict["APN"] = ""

        if conn_info["pass"] != None and conn_info["pass"] != "":
            dict["Clave usuario"] = MSD.MSDUtils.encode_password(conn_info["pass"])
        else:
            dict["Clave usuario"] = ""

        if conn_info["ask_password"] == True:
            dict["Clave al conectar"] = "1"
        else:
            dict["Clave al conectar"] = "0"

        if conn_info["primary_dns"] != None and conn_info["primary_dns"] != "":
            dict["DNS primario"] = conn_info["primary_dns"]
        else:
            dict["DNS primario"] = ''

        if conn_info["secondary_dns"] != None and conn_info["secondary_dns"] != "":
            dict["DNS secundario"] = conn_info["secondary_dns"]
        else:
            dict["DNS secundario"] = ''

        if conn_info["domains"] != None and conn_info["domains"] != "":
            dict["Sufijos DNS"] = conn_info["domains"]
        else:
            dict["Sufijos DNS"] = ""

        if conn_info["proxy"] == True:
            dict["Configuración proxy"] = "1"
            dict["Proxy"] = conn_info["proxy_ip"]
            dict["Puerto proxy"] = conn_info["proxy_port"]
        else:
            dict["Configuración proxy"] = "0"
            dict["Proxy"] = ""
            dict["Puerto proxy"] = "0"

        if conn_info["auto_dns"] == False:
            dict["Usa DNS"] = "1"
        else:
            dict["Usa DNS"] = "0"

        if conn_info["profile_by_default"] == True:
            dict["Usa APN"] = "0"
        else:
            dict["Usa APN"] = "1"

        return dict
