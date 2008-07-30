#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica Mï¿œviles Espaï¿œa S.A.U.
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

import dbus
import dbus.glib
from MSD.MSDPPPvariables import *
import time
import datetime
import os
import gobject
import gtk
import MSD
from MSDUtils import *
import emtraffic


from MobileManager.MobileManagerDbus import MOBILE_MANAGER_CONTROLLER_PATH,MOBILE_MANAGER_CONTROLLER_URI,MOBILE_MANAGER_CONTROLLER_INTERFACE_URI,MOBILE_MANAGER_DEVICE_PATH,MOBILE_MANAGER_DEVICE_URI,MOBILE_MANAGER_DEVICE_INFO_INTERFACE_URI,MOBILE_MANAGER_DEVICE_AUTH_INTERFACE_URI,MOBILE_MANAGER_DEVICE_STATE_INTERFACE_URI,MOBILE_MANAGER_DEVICE_XZONE_INTERFACE_URI


# keep here app-wide traffic history storage object:
# (needs to be reachable by MSDTrafficHistoryGraph, as well as MSDConsumManager;
#  also, could not store it as MSDConsumManager.traffic because MSDConsumWindow
#  cannot access MSDConsumManager --MSDConsumManager.__init__ takes a 
#  consum_window)
traffic = emtraffic.Traffic()
traffic_roaming = emtraffic.Traffic()

STATS_POOLING_INTERVAL = 1000

def get_user_traffic_db_path():
    db_path = os.path.join(os.environ["HOME"],".movistar_desktop","traffic") 
    db_path = os.path.abspath(db_path)
    return db_path

class MSDConsumManager:

    def __init__(self,mcontroller, main_stats_widget,consum_window,conf,delegate=None):
        #FIXED: Controller    
    
    	traffic.set_type("DUN")
    	traffic_roaming.set_type("DUNR")
    	
        self.main_stats_widget = main_stats_widget
        self.mcontroller = mcontroller
        self.consum_window = consum_window
        self.conf = conf
        self.delegate = delegate

        self.roaming = None
        self.running = False
        
        self.session_time_timeout_id =None
        self.session_start_time =0
        self.session_time = 0
        
        self.received_bytes = 0
        self.sent_bytes = 0

        self.last_time = 0
        self.last_received_bytes = 0
        self.last_sent_bytes = 0
        self.last_max_input_speed = 0
        self.last_max_output_speed = 0        
        
        self.last_input_instant_speed =0
        self.last_output_instant_speed = 0
        
        self.initial_acumulated_sent = 0
        self.initial_acumulated_received = 0
        self.initial_acumulated_traffic = 0
        self.initial_acumulated_time = 0
        
        self.max_upload_speed = self.conf.get_max_upload_speed()
        self.consum_window.set_max_upload_speed(self.max_upload_speed)
        self.max_download_speed = self.conf.get_max_download_speed()
        self.consum_window.set_max_download_speed(self.max_download_speed)
        self.in_session_reset_data = 0

        self.apply_timestamp = 0
        
        self.__update_time(0,
                           acumulated_time=self.conf.get_acumulated_time(False),
                           roaming=False)

        self.__update_time(0,
                           acumulated_time=self.conf.get_acumulated_time(True),
                           roaming=True)
        
        self.__update_traffic(0,0,
                              acumulated_received= self.conf.get_acumulated_received_traffic(False),
                              acumulated_sent = self.conf.get_acumulated_sent_traffic(False),
                              acumulated_traffic = self.conf.get_acumulated_traffic(False),
                              roaming=False)
        self.__update_traffic(0,0,
                              acumulated_received= self.conf.get_acumulated_received_traffic(True),
                              acumulated_sent = self.conf.get_acumulated_sent_traffic(True),
                              acumulated_traffic = self.conf.get_acumulated_traffic(True),
                              roaming=True)
        
        self.__update_speed(0,0)

        last_resumen = self.conf.get_last_resumen()
        self.consum_window.consum_resumen_label.set_markup(_(u"<b>Resumen : desde %s</b>" %
                                                             time.strftime("%d/%m/%y - %H:%M", last_resumen)))
	self.consum_window.consum_resumen_label2.set_markup(_(u"<b>Resumen : desde %s</b>" %
                                                             time.strftime("%d/%m/%y - %H:%M", last_resumen)))

        if self.conf.get_clean_resumen() == True :
            self.__clean_needed()
        
        self.consum_window.consum_clean_resumen_checkbutton.set_active(self.conf.get_clean_resumen())
        
        # Warnings

        # set saved values
        self.consum_window.consum_percent_hscale.set_value (self.conf.get_warning_percent())
        self.consum_window.consum_percent_entry.set_value (self.conf.get_warning_percent())
        if self.conf.get_warning_mb_combo() == 3:
            self.consum_window.consum_mb_limit_entry.set_value (self.conf.get_warning_mb(False))
            self.consum_window.consum_mb_limit_entry.set_sensitive(True)
        else:
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)
        self.consum_window.consum_mb_limit_combo.set_active (self.conf.get_warning_mb_combo())
        self.consum_window.consum_mb_warning_chk.set_active (self.conf.get_show_mb_warning(False))
        self.consum_window.consum_ro_mb_warning_chk.set_active (self.conf.get_show_mb_warning(True))
        self.consum_window.consum_ro_mb_limit_entry.set_value (self.conf.get_warning_mb(True))

        
        self.consum_window.consum_percent_warning_chk.set_active (self.conf.get_show_percent_warning())

        # set selected warnings
        self.__update_warnings()

        #signals
        self.consum_window.consum_mb_warning_chk.connect("toggled", self.__mb_warning_cb)
        self.consum_window.consum_ro_mb_warning_chk.connect("toggled", self.__mb_ro_warning_cb)
        self.consum_window.consum_percent_warning_chk.connect("toggled", self.__percent_warning_cb)
        self.consum_window.consum_percent_entry.connect("value-changed", self.__percent_entry_changed_cb)
        self.consum_window.consum_percent_hscale.connect("value_changed", self.__percent_hscale_changed_cb)
        self.consum_window.consum_mb_limit_combo.connect("changed", self.__mb_combo_changed_cb)
        self.consum_window.consum_apply_button.connect("clicked", self.__consum_apply_button_clicked_cb, None)
        self.consum_window.consum_mb_limit_entry.connect("value-changed", self.__mb_entry_changed_cb)
        self.consum_window.consum_ro_mb_limit_entry.connect("value-changed", self.__mb_ro_entry_changed_cb)
        self.consum_window.consum_reset_button.connect("clicked",self.__reset_button_cb)
        self.consum_window.consum_clean_resumen_checkbutton.connect("toggled", self.__clean_resumen_checkbox_cb)

        
        #registrarme a signals del pppd
        self.__register_to_signals()

        self.notified_warning_mb = None
        self.notified_warning_percent = None

        self.notify()
        
    def __consum_apply_button_clicked_cb(self, widget, data):
        print "__consum_apply"
        self.consum_window.consum_apply_button.set_sensitive(False)
        
        if (self.consum_window.consum_mb_warning_chk.get_active()):
            self.conf.set_show_mb_warning(True, False)
        else:
            self.conf.set_show_mb_warning(False, False)
            
        if (self.consum_window.consum_ro_mb_warning_chk.get_active()):
            self.conf.set_show_mb_warning(True, True)
            self.consum_window.consum_ro_mb_limit_entry.update()
            self.conf.set_warning_mb(self.consum_window.consum_ro_mb_limit_entry.get_value(), True)
        else:
            self.conf.set_show_mb_warning(False, True)

        if (self.consum_window.consum_percent_warning_chk.get_active()):
            self.conf.set_show_percent_warning(True)
            self.conf.set_warning_percent(self.consum_window.consum_percent_hscale.get_value())
        else:
            self.conf.set_show_percent_warning(False)

        selected = self.consum_window.consum_mb_limit_combo.get_active()
        
        if selected == 3:
            self.consum_window.consum_mb_limit_entry.update()
            self.conf.set_warning_mb(self.consum_window.consum_mb_limit_entry.get_value(),False)
            self.consum_window.consum_mb_limit_entry.set_sensitive(True)
        else:
            if selected == 0:
                self.conf.set_warning_mb(200, False)
            elif selected == 1:
                self.conf.set_warning_mb(1 * 1024, False)
            elif selected == 2:
                self.conf.set_warning_mb(5 * 1024, False)
            
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)
            
        self.conf.set_warning_mb_combo(selected)        
        
        self.conf.save_conf()
        
        self.__update_warnings()

        

        self.notified_warning_mb = None
        self.notified_warning_percent = None
        self.notify()
        print "__consum_apply (end)"

    def __percent_mb_label_update (self):
        tmp_values = {
            0: 200,
            1: 1024,
            2: (5 * 1024),
            3: self.consum_window.consum_mb_limit_entry.get_value()
            }
        warning_mb = tmp_values.get(self.consum_window.consum_mb_limit_combo.get_active())
        warning_mb_percent = (warning_mb * self.consum_window.consum_percent_entry.get_value()) / 100
        
       #self.consum_window.consum_percent_mb_label.set_text (_(u"Se avisará al consumir %d MB") % warning_mb_percent)

    def __mb_combo_changed_cb(self, combo):
        self.consum_window.consum_apply_button.set_sensitive(True)
        selected = self.consum_window.consum_mb_limit_combo.get_active()
        
        if selected == 3:
            self.consum_window.consum_mb_limit_entry.set_sensitive(True)
        else:
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)

        self.__percent_mb_label_update()
            
    def __percent_hscale_changed_cb(self, range):
        self.consum_window.consum_apply_button.set_sensitive(True)
        self.consum_window.consum_percent_entry.set_value (range.get_value())
        self.__percent_mb_label_update()

    def __percent_entry_changed_cb(self, entry):
        self.consum_window.consum_apply_button.set_sensitive(True)
        self.consum_window.consum_percent_hscale.set_value (entry.get_value())
        self.__percent_mb_label_update()

    def __mb_entry_changed_cb(self, entry):
        self.consum_window.consum_apply_button.set_sensitive(True)
        self.__percent_mb_label_update()

    def __mb_ro_entry_changed_cb(self, entry):
        self.consum_window.consum_apply_button.set_sensitive(True)
        self.__percent_mb_label_update()

    def __update_smbw(self):
        if (self.conf.get_show_mb_warning(False)):
            self.consum_window.consum_mb_warning_chk.set_active(True)
            self.consum_window.consum_mb_limit_combo.set_sensitive(True)
            if self.consum_window.consum_mb_limit_combo.get_active() == 3:
                self.consum_window.consum_mb_limit_entry.set_sensitive(True)
            else:
                self.consum_window.consum_mb_limit_entry.set_sensitive(False)

            ## self.consum_window.consum_warning_image_yes.show()
            ## self.consum_window.consum_warning_image_no.hide()
        else:
            self.consum_window.consum_mb_warning_chk.set_active(False)
            self.consum_window.consum_mb_limit_combo.set_sensitive(False)
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)
            ## self.consum_window.consum_warning_image_yes.hide()
            ## self.consum_window.consum_warning_image_no.show()
            

    def __update_smbw_r(self):
        if (self.conf.get_show_mb_warning(True)):
            self.consum_window.consum_ro_mb_warning_chk.set_active(True)
            self.consum_window.consum_ro_mb_limit_entry.set_sensitive(True)
            ## self.consum_window.consum_r_warning_image_yes.show()
            ## self.consum_window.consum_r_warning_image_no.hide()
        else:
            self.consum_window.consum_ro_mb_warning_chk.set_active(False)
            self.consum_window.consum_ro_mb_limit_entry.set_sensitive(False)
            ## self.consum_window.consum_r_warning_image_yes.hide()
            ## self.consum_window.consum_r_warning_image_no.show()

    def __update_percent(self):
        if self.conf.get_show_mb_warning(False) == True or self.conf.get_show_mb_warning(True) == True :
            if (self.conf.get_show_percent_warning()):
                self.consum_window.consum_percent_warning_chk.set_active(True)
                self.consum_window.consum_percent_hscale.set_sensitive(True)
                self.consum_window.consum_percent_entry.set_sensitive(True)
                self.consum_window.consum_percent_mb_label.set_sensitive(True)
            else:
                self.consum_window.consum_percent_warning_chk.set_active(False)
                self.consum_window.consum_percent_hscale.set_sensitive(False)
                self.consum_window.consum_percent_entry.set_sensitive(False)
                self.consum_window.consum_percent_mb_label.set_sensitive(False)
        else:
            self.consum_window.consum_percent_warning_chk.set_active(False)
            self.consum_window.consum_percent_hscale.set_sensitive(False)
            self.consum_window.consum_percent_entry.set_sensitive(False)
            self.consum_window.consum_percent_mb_label.set_sensitive(False)

        if self.conf.get_show_mb_warning(False) == False and self.conf.get_show_mb_warning(True) == False:
            self.consum_window.consum_percent_warning_chk.set_sensitive(False)
        else:
            self.consum_window.consum_percent_warning_chk.set_sensitive(True)
            

    def __update_warnings(self):
        print "__update_warnings"
        self.__update_smbw()
        self.__update_smbw_r()
        self.__update_percent()

        m_label = ""
        if (self.conf.get_show_mb_warning(False)):
            m_label = _(u"Activado a %s MB "% self.conf.get_warning_mb(False))
            if (self.consum_window.consum_percent_warning_chk.get_active()):
                m_label = _(u"Activado a %s MB y %s MB" % ((float(self.conf.get_warning_mb(False))*float(self.conf.get_warning_percent())/100.0),
                                                          self.conf.get_warning_mb(False)))
                
        r_label = ""
        if (self.conf.get_show_mb_warning(True)):
            r_label = _(u"Activado a %s MB " % self.conf.get_warning_mb(True))
            if (self.consum_window.consum_percent_warning_chk.get_active()):
                 r_label = _(u"Activado a %s MB y %s MB" % ((float(self.conf.get_warning_mb(True))*float(self.conf.get_warning_percent())/100.0),
                                                                     self.conf.get_warning_mb(True)))

        if r_label == "" :
            r_label = _(u"Desactivado")

        if m_label == "" :
            m_label = _(u"Desactivado")
        
        self.consum_window.consum_warning_movistar_label.set_text(m_label)
        self.consum_window.consum_warning_roaming_label.set_text(r_label)

        self.update_color_signals()
            
        print "__update_warnings (end)"

    def __update_temp_smbw(self):
        if (self.consum_window.consum_mb_warning_chk.get_active()):
            self.consum_window.consum_mb_limit_combo.set_sensitive(True)
            if self.consum_window.consum_mb_limit_combo.get_active() == 3:
                self.consum_window.consum_mb_limit_entry.set_sensitive(True)
            else:
                self.consum_window.consum_mb_limit_entry.set_sensitive(False)
        else:
            self.consum_window.consum_mb_warning_chk.set_active(False)
            self.consum_window.consum_mb_limit_combo.set_sensitive(False)
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)
            
    def __update_temp_smbw_r(self):
        if (self.consum_window.consum_ro_mb_warning_chk.get_active()):
            self.consum_window.consum_ro_mb_warning_chk.set_active(True)
            self.consum_window.consum_ro_mb_limit_entry.set_sensitive(True)
        else:
            self.consum_window.consum_ro_mb_warning_chk.set_active(False)
            self.consum_window.consum_ro_mb_limit_entry.set_sensitive(False)
            
    def __update_temp_percent(self):
        if self.consum_window.consum_ro_mb_warning_chk.get_active() == True or self.consum_window.consum_mb_warning_chk.get_active() == True :
            if (self.consum_window.consum_percent_warning_chk.get_active()):
                self.consum_window.consum_percent_warning_chk.set_active(True)
                self.consum_window.consum_percent_hscale.set_sensitive(True)
                self.consum_window.consum_percent_entry.set_sensitive(True)
                self.consum_window.consum_percent_mb_label.set_sensitive(True)
            else:
                self.consum_window.consum_percent_warning_chk.set_active(False)
                self.consum_window.consum_percent_hscale.set_sensitive(False)
                self.consum_window.consum_percent_entry.set_sensitive(False)
                self.consum_window.consum_percent_mb_label.set_sensitive(False)
        else:
            self.consum_window.consum_percent_warning_chk.set_active(False)
            self.consum_window.consum_percent_hscale.set_sensitive(False)
            self.consum_window.consum_percent_entry.set_sensitive(False)
            self.consum_window.consum_percent_mb_label.set_sensitive(False)

        if self.consum_window.consum_ro_mb_warning_chk.get_active() == True or self.consum_window.consum_mb_warning_chk.get_active() == True :
            print "---> chk %s" % False
            self.consum_window.consum_percent_warning_chk.set_sensitive(True)
        else:
            print "---> chk %s" % True
            self.consum_window.consum_percent_warning_chk.set_sensitive(False)

    def __update_warnings_temporal(self):
        print "__update_warnings_temp"
        self.__update_temp_smbw()
        self.__update_temp_smbw_r()
        self.__update_temp_percent() 
        print "__update_warnings_temp (end)"

    def __mb_warning_cb(self, widget):
        self.consum_window.consum_apply_button.set_sensitive(True)
        ## if self.consum_window.consum_mb_warning_chk.get_active() == False:
        ##     self.consum_window.consum_percent_warning_chk.set_active(False)
        ## else:
        ##     self.consum_window.consum_percent_warning_chk.set_sensitive(True)
        
        self.__update_warnings_temporal()

    def __mb_ro_warning_cb(self, widget):
        self.consum_window.consum_apply_button.set_sensitive(True)
        ## if self.consum_window.consum_ro_mb_warning_chk.get_active() == False:
        ##     self.consum_window.consum_percent_warning_chk.set_active(False)
        ## else:
        ##     self.consum_window.consum_percent_warning_chk.set_sensitive(True)
        
        self.__update_warnings_temporal()


    def __percent_warning_cb(self, widget):
        self.consum_window.consum_apply_button.set_sensitive(True)
        
        self.__update_warnings_temporal()

    def __register_to_signals(self):
        #FIXED : Dialer
        if self.mcontroller.dialer != None :
            # signals
            self.ppp_manager = self.mcontroller.dialer
            self.ppp_manager.connect("connected", self.__connected_cb)
            self.ppp_manager.connect("disconnected", self.__disconnected_cb)
            self.ppp_manager.connect("connecting", self.__connecting_cb)
            self.ppp_manager.connect("disconnecting", self.__disconnecting_cb)
            self.ppp_manager.connect("pppstats_signal", self.__ppp_stats_cb)

    def isRoaming(self):
        if self.roaming != None :
            return self.roaming
        
        dev = self.mcontroller.get_active_device()
        if dev != None :
            odev = self.mcontroller.get_device_obj_from_path(dev)
            self.roaming = odev.is_roaming()
            return self.roaming
        
        return False
		
    def __connected_cb(self, dialer):        
        self.session_start_time = time.time()
        self.last_time = self.session_start_time
        self.last_received_bytes = 0
        self.last_sent_bytes = 0
        self.last_max_input_speed = 0
        self.last_max_output_speed = 0
           
        self.last_input_instant_speed =0
        self.last_output_instant_speed = 0
        
        self.initial_acumulated_sent = self.conf.get_acumulated_sent_traffic(self.isRoaming())
        self.initial_acumulated_received = self.conf.get_acumulated_received_traffic(self.isRoaming())
        self.initial_acumulated_traffic = self.conf.get_acumulated_traffic(self.isRoaming())
        self.initial_acumulated_time = self.conf.get_acumulated_time(self.isRoaming())
        
        self.max_upload_speed = self.conf.get_max_upload_speed()
        self.max_download_speed = self.conf.get_max_download_speed()

        self.notified_warning_mb = None
        self.notified_warning_percent = None
        
        self.session_time_timeout_id = gobject.timeout_add(STATS_POOLING_INTERVAL,self.__timeout_cb)
        self.consum_window.graph.reset()
        self.consum_window.set_connection_type(self.isRoaming())

        self.running = True
        
        
    def __connecting_cb(self, dialer):
        pass

    def __disconnected_cb(self, dialer):
        #This case is used for close app
        if self.running == False:
            return
        
        self.consum_window.set_connection_type()
        self.roaming = None
        self.running = False
        
    def __disconnecting_cb(self, dialer):
        if self.running == False:
            return
        
        if self.session_time_timeout_id is None:
            return

        in_traffic = self.last_received_bytes
        out_traffic = self.last_sent_bytes
        self.last_received_bytes = 0
        self.last_sent_bytes = 0
        
        # notifico al delegate
        if self.delegate is not None and hasattr(self.delegate, "consum_session_updated"):
            self.delegate.consum_session_updated(in_traffic,out_traffic)
        
        self.session_start_time = 0
        self.session_time = 0
        self.last_max_output_speed = 0
        self.last_max_input_speed = 0
        
        gobject.source_remove(self.session_time_timeout_id)
        self.__update_time(0, roaming=self.isRoaming())
        self.__update_traffic(0,0,roaming=self.isRoaming())
        
        self.__update_speed(0,0)
        self.consum_window.set_maximun_output_speed(0)
        self.consum_window.set_maximun_input_speed(0)
        self.consum_window.graph.reset()
        
        self.end_history()
        
        print _(u"process history disconnecting")

    def end_history(self):
        print "---> End History"
        if self.isRoaming():
        	traffic_roaming.process_history_traffic(0, 0, int(os.times()[4]*1000.0), True)
        else:
        	traffic.process_history_traffic(0, 0, int(os.times()[4]*1000.0), True)
        
        self.consum_window.history_graph.reload_data()

    def notify(self, new_acumulated_traffic=None):
        self.update_color_signals()
        
        if new_acumulated_traffic == None:
            new_acumulated_traffic = self.conf.get_acumulated_traffic(self.isRoaming())
        # chequear avisos
        if new_acumulated_traffic > self.conf.get_warning_mb(self.isRoaming()) * 1024 * 1024:
            if self.notified_warning_mb != self.conf.get_warning_mb(self.isRoaming()):
                if self.conf.get_show_mb_warning(self.isRoaming()) == True:
                    show_notification (_(u"Consumo"),
                                       _(u"Se ha alcanzado el límite de consumo GPRS/3G fijado"),
				       icon=MSD.icons_files_dir + "consumo_32x32.png")
                self.notified_warning_mb = self.conf.get_warning_mb(self.isRoaming())
        else:        
            mb_percent_limit = (self.conf.get_warning_mb(self.isRoaming()) * self.conf.get_warning_percent() * 1024 * 1024) / 100
            if new_acumulated_traffic > mb_percent_limit:
                if self.notified_warning_percent != self.conf.get_warning_percent():
                    if self.conf.get_show_percent_warning() == True:
                        show_notification (_(u"Consumo"),
                                           _(u"Se ha alcanzado el tanto por ciento del límite de consumo GPRS/3G fijado"),
				       	   icon=MSD.icons_files_dir + "consumo_32x32.png")
                    self.notified_warning_percent = self.conf.get_warning_percent()

    def update_color_signals(self):
        new_acumulated_traffic = self.conf.get_acumulated_traffic(False)
        
        new_r_acumulated_traffic = self.conf.get_acumulated_traffic(True)

        if self.conf.get_show_mb_warning(False) == False:
            self.consum_window.consum_warning_image.set_from_file(MSD.icons_files_dir + "grey_signal.png")
        else:
            if self.conf.get_show_percent_warning() == True:
                if new_acumulated_traffic > self.conf.get_warning_mb(False) * 1024 * 1024:
                    self.consum_window.consum_warning_image.set_from_file(MSD.icons_files_dir + "red_signal.png")
                else:
                    mb_percent_limit = (self.conf.get_warning_mb(False) * self.conf.get_warning_percent() * 1024 * 1024) / 100
                    if new_acumulated_traffic > mb_percent_limit:
                        self.consum_window.consum_warning_image.set_from_file(MSD.icons_files_dir + "yellow_signal.png")
                    else:
                        self.consum_window.consum_warning_image.set_from_file(MSD.icons_files_dir + "green_signal.png")
            else:
                if new_acumulated_traffic > self.conf.get_warning_mb(False) * 1024 * 1024:
                    self.consum_window.consum_warning_image.set_from_file(MSD.icons_files_dir + "red_signal.png")
                else:
                    self.consum_window.consum_warning_image.set_from_file(MSD.icons_files_dir + "green_signal.png")

        if self.conf.get_show_mb_warning(True) == False:
            self.consum_window.consum_r_warning_image.set_from_file(MSD.icons_files_dir + "grey_signal.png")
        else:
            if self.conf.get_show_percent_warning() == True:
                if new_r_acumulated_traffic > self.conf.get_warning_mb(True) * 1024 * 1024:
                    self.consum_window.consum_r_warning_image.set_from_file(MSD.icons_files_dir + "red_signal.png")
                else:
                    mb_percent_limit = (self.conf.get_warning_mb(True) * self.conf.get_warning_percent() * 1024 * 1024) / 100
                    if new_r_acumulated_traffic > mb_percent_limit:
                        self.consum_window.consum_r_warning_image.set_from_file(MSD.icons_files_dir + "yellow_signal.png")
                    else:
                        self.consum_window.consum_r_warning_image.set_from_file(MSD.icons_files_dir + "green_signal.png")
            else:
                if new_r_acumulated_traffic > self.conf.get_warning_mb(True) * 1024 * 1024:
                    self.consum_window.consum_r_warning_image.set_from_file(MSD.icons_files_dir + "red_signal.png")
                else:
                    self.consum_window.consum_r_warning_image.set_from_file(MSD.icons_files_dir + "green_signal.png")
            
        
    def __ppp_stats_cb(self, dialer, received_bytes, sent_bytes, interval_time):
        print "MSDConsumManager -> __ppp_stats_cb"
        first_time = False
        #calculo velocidades
        if self.last_received_bytes == 0:
            first_time = True
            
        received_delta = received_bytes - self.last_received_bytes
        sent_delta = sent_bytes - self.last_sent_bytes
        if received_bytes < 0 :
            received_delta = 0
        if sent_delta < 0 :
            sent_delta = 0
            
            
        now = time.time()            
        secs_between_measures = interval_time

        
        instant_input_speed = received_delta  / secs_between_measures
        instant_output_speed = sent_delta  / secs_between_measures        
    


        
        # grafica
        instant_output_speed_kbits = (instant_output_speed * 8) / 1024
        instant_input_speed_kbits = (instant_input_speed * 8) / 1024
        print _(u"Rcv %s bytes  in %s seconds speed = %s") % (received_delta, secs_between_measures,instant_input_speed_kbits)

        if not first_time:
            self.consum_window.graph.set_in_value(instant_input_speed_kbits)
            self.consum_window.graph.set_out_value(instant_output_speed_kbits)

            # etiquetas 
            self.__update_speed(instant_input_speed ,instant_output_speed)

        
        self.last_received_bytes = received_bytes
        self.last_sent_bytes = sent_bytes

           
        # notifico al delegate
        if self.delegate is not None and hasattr(self.delegate, "consum_session_updated"):
            print _(u"Notificando delegado")
            self.delegate.consum_session_updated(received_bytes,sent_bytes)

        # calculo los nuevos acumulados
        new_acumulated_received = self.conf.get_acumulated_received_traffic(self.isRoaming())  + received_delta
        new_acumulated_sent = self.conf.get_acumulated_sent_traffic(self.isRoaming()) + sent_delta
        new_acumulated_traffic = self.conf.get_acumulated_traffic(self.isRoaming()) + (sent_delta + received_delta)

        self.notify(new_acumulated_traffic)

        self.conf.set_acumulated_sent_traffic(new_acumulated_sent, self.isRoaming())
        self.conf.set_acumulated_received_traffic(new_acumulated_received, self.isRoaming())
        self.conf.set_acumulated_traffic(new_acumulated_traffic, self.isRoaming())
        self.conf.save_conf()
        
        self.__update_traffic(received_bytes,sent_bytes,
                              new_acumulated_received,new_acumulated_sent, new_acumulated_traffic,
                              self.isRoaming())
        
        if self.isRoaming():
            traffic_roaming.process_history_traffic(sent_delta, received_delta, int(os.times()[4]*1000.0), False)
        else:
            traffic.process_history_traffic(sent_delta, received_delta, int(os.times()[4]*1000.0), False)
        
        # print "process history traffic : %s %s %s" % (sent_bytes, received_bytes, int(time.clock()*1000.0))
        
    def __timeout_cb(self):
        try:
            new_time = time.time()
            time_delta = new_time - self.last_time
            self.last_time = new_time
            session_time = new_time - self.session_start_time
            new_acumulated_time = self.conf.get_acumulated_time(self.isRoaming()) + time_delta
            self.conf.set_acumulated_time(new_acumulated_time,self.isRoaming())
            self.conf.save_conf()
            self.__update_time(session_time,new_acumulated_time, roaming=self.isRoaming())
        finally:
            return True


    def __update_time(self,session_time,acumulated_time=None,roaming=False):
        self.session_time = session_time
        self.main_stats_widget.set_session_time(session_time)
        self.consum_window.set_session_time(session_time)
        if acumulated_time is not None:
            self.consum_window.set_acumulated_time(acumulated_time, roaming)
        
    def __update_traffic(self,received_bytes,sent_bytes,acumulated_received=None,
                         acumulated_sent=None, acumulated_traffic=None,
                         roaming=False):
        
        self.main_stats_widget.set_session_traffic(received_bytes + sent_bytes)
        self.consum_window.set_session_traffic(received_bytes,sent_bytes)

        if acumulated_received is not None:            
            self.consum_window.set_acumulated_received(acumulated_received, roaming)

        if acumulated_sent is not None:
            self.consum_window.set_acumulated_sent(acumulated_sent, roaming)

        if acumulated_traffic is not None:
            self.consum_window.set_acumulated_traffic(acumulated_traffic, roaming)

        
    def __update_speed(self,input_speed,output_speed):
        self.main_stats_widget.set_instant_speed(input_speed+output_speed)

        if output_speed >= self.last_max_output_speed:
            self.last_max_output_speed = output_speed
            self.consum_window.set_maximun_output_speed(output_speed)

        if output_speed >= self.max_upload_speed:
            self.max_upload_speed = output_speed
            self.conf.set_max_upload_speed(output_speed)
            self.conf.save_conf()
            self.consum_window.set_max_upload_speed(output_speed)

        if input_speed >= self.last_max_input_speed:
            self.last_max_input_speed = input_speed
            self.consum_window.set_maximun_input_speed(input_speed)

        if input_speed >= self.max_download_speed:
            self.max_download_speed = input_speed
            self.conf.set_max_download_speed(input_speed)
            self.conf.save_conf()
            self.consum_window.set_max_download_speed(input_speed)

    def __clean_resumen_checkbox_cb(self, widget):
        value = widget.get_active()
        if value == True :
            self.__clean_needed()
        self.conf.set_clean_resumen(value)

    def __clean_needed(self):
        last_resumen =  self.conf.get_last_resumen()
        last_resumen_y = last_resumen[0]
        last_resumen_m = last_resumen[1]
        last_resumen_d = last_resumen[2]

        today = time.localtime()
        today_y = today[0]
        today_m = today[1]
        today_d = today[2]

        ciclo_y = today_y
        ciclo_m = None
        ciclo_d = 18
        
        if today_d >= ciclo_d :
            ciclo_m = today_m
        else:
            ciclo_m = today_m - 1

        last_date = datetime.datetime(last_resumen_y, last_resumen_m, last_resumen_d)
        ciclo_date = datetime.datetime(ciclo_y, ciclo_m, ciclo_d)

        if last_date < ciclo_date :
            self.__reset_acumulado()

    def __reset_acumulado(self):
        self.conf.reset_acumulated()
        self.initial_acumulated_sent = 0
        self.initial_acumulated_received = 0
        self.initial_acumulated_traffic = 0
        self.initial_acumulated_time = 0
        self.max_upload_speed = 0
        self.max_download_speed = 0
        self.consum_window.set_acumulated_time(0, False)
        self.consum_window.set_acumulated_received(0, False)
        self.consum_window.set_acumulated_traffic(0, False)
        self.consum_window.set_acumulated_sent(0, False)
        
        self.consum_window.set_acumulated_time(0, True)
        self.consum_window.set_acumulated_received(0, True)
        self.consum_window.set_acumulated_traffic(0, True)
        self.consum_window.set_acumulated_sent(0, True)
        
        last_resumen = self.conf.get_last_resumen()
        self.consum_window.consum_resumen_label.set_markup(_(u"<b>Resumen : desde %s</b>" %
                                                             time.strftime("%d/%m/%y - %H:%M", last_resumen)))
	self.consum_window.consum_resumen_label2.set_markup(_(u"<b>Resumen : desde %s</b>" %
                                                             time.strftime("%d/%m/%y - %H:%M", last_resumen)))
        self.conf.set_max_upload_speed(self.last_max_output_speed)
        self.conf.set_max_download_speed(self.last_max_input_speed)
        self.conf.save_conf()
        self.consum_window.set_max_upload_speed(self.last_max_output_speed)
        self.consum_window.set_max_download_speed(self.last_max_input_speed)
        
        self.notified_warning_mb = None
        self.notified_warning_percent = None
        
        self.update_color_signals()
    
    def __reset_button_cb(self,widget):
        mesg = gtk.MessageDialog(type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK_CANCEL)
        mesg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        mesg.set_markup(_(u"<b>Reiniciar consumo acumulado</b>"))
        mesg.format_secondary_markup(MSD.MSG_RESET_CONSUM_WARNING)
        if (mesg.run() == gtk.RESPONSE_OK):
           self.__reset_acumulado()
        
        mesg.destroy()
        return False

