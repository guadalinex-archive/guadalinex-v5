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


import math
import base64
import os
import gtk
import popen2
import MSD

decode_key = 'EnUnLugarDeLaMancha'

def show_notification(title, msg, urgency="normal", seconds=5000, icon="movistar_icon_notify"):
    if (os.path.exists ("/usr/bin/notify-send") == False):
        if (os.path.exists ("/opt/gnome/bin/notify-send") == False):
            return

    arguments = "-u " + urgency
    arguments += " -t " + str(seconds)
    if icon:
        arguments += " -i " + icon
    arguments += " \"" + title + "\" " + "\"" + msg + "\""

    os.system("notify-send " + arguments)

def format_to_maximun_unit(number,*args):
    n = int(number)
    base = 1024
    #"GBytes","MBytes","Kbytes","bytes")
    max_exponent = len(args) +1
    # determinoo la maxima unidad
    exponent = 0
    for i in range(1,max_exponent):
        exponent = i
        if number <  math.pow(base,i):
            break
        
    units = list(args)
    units.reverse()
    new_value = float(number) / math.pow(base,exponent-1)
    return "%.2f %s"%(new_value,units[exponent-1]) 

def seconds_to_hours_minutes_seconds(seconds):    
    try:
        secs = int(seconds)
        h = secs / (3600)
        t = secs - (h * 3600)
        m = t / 60
        s = t - (m * 60)
        t = secs - (h * 3600)
        return h,m,s
    except Exception ,msg:
        return -1,-1,-1
    


def seconds_to_time_string(seconds):
    h,m,s = seconds_to_hours_minutes_seconds(seconds)
    return  "%02d:%02d:%02d" %(h,m,s)




def decode_password(encrypted_password_string):
        '''
        La password no se encuentra en claro en los ficheros exportados por ellso hay
        que decodificarla. Esta funcion toma una cadena como parametro que representa
        la password tal como se encuentra escrita en el fichero de servicio
        '''
        
        encrypted_password = encrypted_password_string.strip()
        if encrypted_password == "":
            return ""
        
        # convertir en una cadena  base64 valida
        tmp_buf = encrypted_password.replace("*","=")
        #decode base 64                                    
        tmp_buf = base64.decodestring(tmp_buf)
        #XOR 

        key_size = len(decode_key)
        clear_password = ''
        
        for i in range(len(tmp_buf)):
            ch1 = ord(tmp_buf[i])
            ch2 = ord(decode_key[i % key_size])
            clear_ch = chr(ch1 ^ ch2)
            clear_password += str(clear_ch)
        return clear_password.strip("\n")

def encode_password(password):
        '''
        Codifica la password para no escribirla en claro en los ficheros exportados
        '''
        #XOR
        tmp_buf = ""
        key_size = len(decode_key)
        for i in range(len(password)):
            ch1 =  ord(password[i])
            ch2 =  ord(decode_key[i % key_size])  
            tmp_ch = chr(ch1 ^ ch2)
            tmp_buf += str(tmp_ch)
        
        #base64 encode
        tmp_buf = base64.encodestring(tmp_buf)
        #cambiar = por *
        tmp_buf = tmp_buf.replace("=","*")
        return tmp_buf.strip("\n")

def close_app(conn_manager, conf, updater):
    set_active_dev_by_default(conn_manager.mcontroller)

    if updater.process != None:
        if updater.process.poll() == -1:
            os.system("kill -9 %d" % updater.process.pid)

    if conn_manager.close_app() == True:
        for dev in conn_manager.mcontroller.get_available_devices() :
            dev.close_device()
            print "Close Device : %s" % dev.dev_props["info.product"]
        gtk.main_quit()
        return False
    else:
        return True

def set_active_dev_by_default(mcontroller):
    dev = mcontroller.get_active_device()
    if dev != None:
        fd = open (MSD.default_device_file, "w")
        fd.write(dev.dev_props["info.udi"])
        print "set by default -> %s" % dev.dev_props["info.udi"]
        fd.close()
    
