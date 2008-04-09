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

import dbus
import dbus.glib
from PPPManager import *
import time
import gobject
import gtk
import MSD
from MSDUtils import *

STATS_POOLING_INTERVAL = 1000

class MSDConsumManager:

    def __init__(self,mcontroller, main_stats_widget,consum_window,conf,delegate=None):
        self.main_stats_widget = main_stats_widget
        self.mcontroller = mcontroller
        self.consum_window = consum_window
        self.conf = conf
        self.delegate = delegate
        
        self.session_time_timeout_id =None
        self.session_start_time =0
        self.session_time = 0
        
        self.recived_bytes = 0
        self.sent_bytes = 0

        self.last_time = 0
        self.last_recived_bytes = 0
        self.last_sent_bytes = 0
        self.last_max_input_speed = 0
        self.last_max_output_speed = 0        
        
        self.last_input_instant_speed =0
        self.last_output_instant_speed = 0
        
        self.initial_acumulated_sent = self.conf.get_acumulated_sent_traffic()
        self.initial_acumulated_recived = self.conf.get_acumulated_recived_traffic()
        self.initial_acumulated_traffic = self.conf.get_acumulated_traffic()
        self.initial_acumulated_time = self.conf.get_acumulated_time()
        self.max_upload_speed = self.conf.get_max_upload_speed()
        self.consum_window.set_max_upload_speed(self.max_upload_speed)
        self.max_download_speed = self.conf.get_max_download_speed()
        self.consum_window.set_max_download_speed(self.max_download_speed)
        self.in_session_reset_data = 0

        self.apply_timestamp = 0
        
        self.__update_time(0,self.conf.get_acumulated_time())
        self.__update_traffic(0,0,self.initial_acumulated_recived, self.initial_acumulated_sent, self.initial_acumulated_traffic)
        self.__update_speed(0,0)
        self.consum_window.consum_reset_button.connect("clicked",self.__reset_button_cb)

        # Warnings

        # set saved values
        self.consum_window.consum_percent_hscale.set_value (self.conf.get_warning_percent())
        self.consum_window.consum_percent_entry.set_value (self.conf.get_warning_percent())
        if self.conf.get_warning_mb_combo() == 3:
            self.consum_window.consum_mb_limit_entry.set_value (self.conf.get_warning_mb())
            self.consum_window.consum_mb_limit_entry.set_sensitive(True)
        else:
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)
        self.consum_window.consum_mb_limit_combo.set_active (self.conf.get_warning_mb_combo())
        self.consum_window.consum_mb_warning_chk.set_active (self.conf.get_show_mb_warning())
        self.consum_window.consum_percent_warning_chk.set_active (self.conf.get_show_percent_warning())

        # set selected warnings
        self.__update_warnings()

        #signals
        self.consum_window.consum_mb_warning_chk.connect("toggled", self.__mb_warning_cb)
        self.consum_window.consum_percent_warning_chk.connect("toggled", self.__percent_warning_cb)
        self.consum_window.consum_percent_entry.connect("value-changed", self.__percent_entry_changed_cb)
        self.consum_window.consum_percent_hscale.connect("value_changed", self.__percent_hscale_changed_cb)
        self.consum_window.consum_mb_limit_combo.connect("changed", self.__mb_combo_changed_cb)
        self.consum_window.consum_apply_button.connect("clicked", self.__consum_apply_button_clicked_cb, None)
        self.consum_window.consum_mb_limit_entry.connect("value-changed", self.__mb_entry_changed_cb)
        
        #registrarme a signals del pppd
        self.__register_to_signals()

        self.notified_warning_mb = None
        self.notified_warning_percent = None

        self.notify()
        
    def __consum_apply_button_clicked_cb(self, widget, data):
        self.consum_window.consum_apply_button.set_sensitive(False)
        self.consum_window.consum_mb_limit_entry.update()
        self.conf.set_warning_mb(self.consum_window.consum_mb_limit_entry.get_value())
        self.conf.set_warning_percent(self.consum_window.consum_percent_hscale.get_value())
        if (self.consum_window.consum_mb_warning_chk.get_active()):
            self.conf.set_show_mb_warning(True)
        else:
            self.conf.set_show_mb_warning(False)

        if (self.consum_window.consum_percent_warning_chk.get_active()):
            self.conf.set_show_percent_warning(True)
        else:
            self.conf.set_show_percent_warning(False)

        selected = self.consum_window.consum_mb_limit_combo.get_active()
        
        if selected == 3:
            self.consum_window.consum_mb_limit_entry.set_sensitive(True)
        else:
            if selected == 0:
                self.conf.set_warning_mb(200)
            elif selected == 1:
                self.conf.set_warning_mb(1 * 1024)
            elif selected == 2:
                self.conf.set_warning_mb(5 * 1024)

            #self.consum_window.consum_mb_limit_entry.set_text(str(1))
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)
            
        self.conf.set_warning_mb_combo(selected)
        
        self.conf.save_conf()
        self.__update_warnings()

        self.notified_warning_mb = None
        self.notified_warning_percent = None
        self.notify()

    def __percent_mb_label_update (self):
        tmp_values = {
            0: 200,
            1: 1024,
            2: (5 * 1024),
            3: self.consum_window.consum_mb_limit_entry.get_value()
            }
        warning_mb = tmp_values.get(self.consum_window.consum_mb_limit_combo.get_active())
        warning_mb_percent = (warning_mb * self.consum_window.consum_percent_entry.get_value()) / 100
        self.consum_window.consum_percent_mb_label.set_text (u"Se avisará al consumir %d MB" % warning_mb_percent)

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

    def __update_warnings(self):
        if (self.conf.get_show_mb_warning()):
            self.consum_window.consum_mb_warning_chk.set_active(True)
            self.consum_window.consum_mb_limit_combo.set_sensitive(True)
            if self.consum_window.consum_mb_limit_combo.get_active() == 3:
                self.consum_window.consum_mb_limit_entry.set_sensitive(True)
            else:
                self.consum_window.consum_mb_limit_entry.set_sensitive(False)

            self.consum_window.consum_percent_warning_chk.set_sensitive(True)
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
            self.consum_window.consum_mb_warning_chk.set_active(False)
            self.consum_window.consum_mb_limit_combo.set_sensitive(False)
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)
            self.consum_window.consum_percent_warning_chk.set_sensitive(False)
            self.consum_window.consum_percent_hscale.set_sensitive(False)
            self.consum_window.consum_percent_entry.set_sensitive(False)
            self.consum_window.consum_percent_mb_label.set_sensitive(False)

    def __update_warnings_temporal(self):
        if (self.consum_window.consum_mb_warning_chk.get_active()):
            self.consum_window.consum_mb_warning_chk.set_active(True)
            self.consum_window.consum_mb_limit_combo.set_sensitive(True)
            if self.consum_window.consum_mb_limit_combo.get_active() == 3:
                self.consum_window.consum_mb_limit_entry.set_sensitive(True)
            else:
                self.consum_window.consum_mb_limit_entry.set_sensitive(False)

            self.consum_window.consum_percent_warning_chk.set_sensitive(True)
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
            self.consum_window.consum_mb_warning_chk.set_active(False)
            self.consum_window.consum_mb_limit_combo.set_sensitive(False)
            self.consum_window.consum_mb_limit_entry.set_sensitive(False)
            self.consum_window.consum_percent_warning_chk.set_sensitive(False)
            self.consum_window.consum_percent_hscale.set_sensitive(False)
            self.consum_window.consum_percent_entry.set_sensitive(False)
            self.consum_window.consum_percent_mb_label.set_sensitive(False)

    def __mb_warning_cb(self, widget):
        self.consum_window.consum_apply_button.set_sensitive(True)
        if self.consum_window.consum_mb_warning_chk.get_active() == False:
            self.consum_window.consum_percent_warning_chk.set_active(False)
        self.__update_warnings_temporal()


    def __percent_warning_cb(self, widget):
        self.consum_window.consum_apply_button.set_sensitive(True)
        
        self.__update_warnings_temporal()

    def __register_to_signals(self):
        if self.mcontroller.dialer != None :
            # signals
            self.ppp_manager = self.mcontroller.dialer
            self.ppp_manager.connect("connected", self.__connected_cb)
            self.ppp_manager.connect("disconnected", self.__disconnected_cb)
            self.ppp_manager.connect("connecting", self.__connecting_cb)
            self.ppp_manager.connect("disconnecting", self.__disconnecting_cb)
            self.ppp_manager.connect("pppstats_signal", self.__ppp_stats_cb)

    def __connected_cb(self, dialer):        
        self.session_start_time = time.time()
        self.last_time = self.session_start_time
        self.last_recived_bytes = 0
        self.last_sent_bytes = 0
        self.last_max_input_speed = 0
        self.last_max_output_speed = 0
           
        self.last_input_instant_speed =0
        self.last_output_instant_speed = 0
        
        self.initial_acumulated_sent = self.conf.get_acumulated_sent_traffic()
        self.initial_acumulated_recived = self.conf.get_acumulated_recived_traffic()
        self.initial_acumulated_traffic = self.conf.get_acumulated_traffic()
        self.initial_acumulated_time = self.conf.get_acumulated_time()
        self.max_upload_speed = self.conf.get_max_upload_speed()
        self.max_download_speed = self.conf.get_max_download_speed()

        self.notified_warning_mb = None
        self.notified_warning_percent = None
        
        self.session_time_timeout_id = gobject.timeout_add(STATS_POOLING_INTERVAL,self.__timeout_cb)
        self.consum_window.graph.reset()
        
    def __connecting_cb(self, dialer):
        pass

    def __disconnected_cb(self, dialer):
        pass

        
    def __disconnecting_cb(self, dialer):
        if self.session_time_timeout_id is None:
            return

        in_traffic = self.last_recived_bytes
        out_traffic = self.last_sent_bytes
        self.last_recived_bytes = 0
        self.last_sent_bytes = 0
        
        # notifico al delegate
        if self.delegate is not None and hasattr(self.delegate, "consum_session_updated"):
            self.delegate.consum_session_updated(in_traffic,out_traffic)
        
        self.session_start_time = 0
        self.session_time = 0
        self.last_max_output_speed = 0
        self.last_max_input_speed = 0
        
        gobject.source_remove(self.session_time_timeout_id)
        self.__update_time(0)
        self.__update_traffic(0,0)
        self.__update_speed(0,0)
        self.consum_window.set_maximun_output_speed(0)
        self.consum_window.set_maximun_input_speed(0)
        self.consum_window.graph.reset()

    def notify(self, new_acumulated_traffic=None):
        if new_acumulated_traffic == None:
            new_acumulated_traffic = self.conf.get_acumulated_traffic()
        # chequear avisos
        if new_acumulated_traffic > self.conf.get_warning_mb() * 1024 * 1024:
            if self.notified_warning_mb != self.conf.get_warning_mb():
                if self.conf.get_show_mb_warning() == True:
                    show_notification ("Consumo",
                                       u"Se ha alcanzado el límite de consumo GPRS/3G fijado")
                self.notified_warning_mb = self.conf.get_warning_mb()
        else:        
            mb_percent_limit = (self.conf.get_warning_mb() * self.conf.get_warning_percent() * 1024 * 1024) / 100
            if new_acumulated_traffic > mb_percent_limit:
                if self.notified_warning_percent != self.conf.get_warning_percent():
                    if self.conf.get_show_percent_warning() == True:
                        show_notification ("Consumo",
                                           u"Se ha alcanzado el tanto por ciento del límite de consumo GPRS/3G fijado")
                    self.notified_warning_percent = self.conf.get_warning_percent()
        
    def __ppp_stats_cb(self, dialer, recived_bytes, sent_bytes, interval_time):
        print "MSDConsumManager -> __ppp_stats_cb"
        first_time = False
        #calculo velocidades
        if self.last_recived_bytes == 0:
            first_time = True
            
        recived_delta = recived_bytes - self.last_recived_bytes
        sent_delta = sent_bytes - self.last_sent_bytes
        if recived_bytes < 0 :
            recived_delta = 0
        if sent_delta < 0 :
            sent_delta = 0
            
            
        now = time.time()            
        secs_between_measures = interval_time

        
        instant_input_speed = recived_delta  / secs_between_measures
        instant_output_speed = sent_delta  / secs_between_measures        
    


        
        # grafica
        instant_output_speed_kbits = (instant_output_speed * 8) / 1024
        instant_input_speed_kbits = (instant_input_speed * 8) / 1024
        print "Rcv %s bytes  in %s seconds speed = %s" % (recived_delta, secs_between_measures,instant_input_speed_kbits)

        if not first_time:
            self.consum_window.graph.set_in_value(instant_input_speed_kbits)
            self.consum_window.graph.set_out_value(instant_output_speed_kbits)

            # etiquetas 
            self.__update_speed(instant_input_speed ,instant_output_speed)

        
        self.last_recived_bytes = recived_bytes
        self.last_sent_bytes = sent_bytes

           
        # notifico al delegate
        if self.delegate is not None and hasattr(self.delegate, "consum_session_updated"):
            print "Notificando delegado"
            self.delegate.consum_session_updated(recived_bytes,sent_bytes)

        # calculo los nuevos acumulados
        new_acumulated_recived = self.conf.get_acumulated_recived_traffic()  + recived_delta
        new_acumulated_sent = self.conf.get_acumulated_sent_traffic() + sent_delta
        new_acumulated_traffic = self.conf.get_acumulated_traffic() + (sent_delta + recived_delta)

        self.notify(new_acumulated_traffic)

        self.conf.set_acumulated_sent_traffic(new_acumulated_sent)
        self.conf.set_acumulated_recived_traffic(new_acumulated_recived)
        self.conf.set_acumulated_traffic(new_acumulated_traffic)
        self.conf.save_conf()
        
        self.__update_traffic(recived_bytes,sent_bytes,new_acumulated_recived,new_acumulated_sent, new_acumulated_traffic)
        
    def __timeout_cb(self):
        try:
            new_time = time.time()
            time_delta = new_time - self.last_time
            self.last_time = new_time
            session_time = new_time - self.session_start_time
            new_acumulated_time = self.conf.get_acumulated_time()  + time_delta
            self.conf.set_acumulated_time(new_acumulated_time)
            self.conf.save_conf()
            self.__update_time(session_time,new_acumulated_time)
        finally:
            return True


    def __update_time(self,session_time,acumulated_time=None):
        self.session_time = session_time
        self.main_stats_widget.set_session_time(session_time)
        self.consum_window.set_session_time(session_time)
        if acumulated_time is not None:
            self.consum_window.set_acumulated_time(acumulated_time)
        
    def __update_traffic(self,recived_bytes,sent_bytes,acumulated_recived=None,acumulated_sent=None, acumulated_traffic=None):
        self.main_stats_widget.set_session_traffic(recived_bytes + sent_bytes)
        self.consum_window.set_session_traffic(recived_bytes,sent_bytes)

        if acumulated_recived is not None:            
            self.consum_window.set_acumulated_recived(acumulated_recived)

        if acumulated_sent is not None:
            self.consum_window.set_acumulated_sent(acumulated_sent)

        if acumulated_traffic is not None:
            self.consum_window.set_acumulated_traffic(acumulated_traffic)

        
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

    
    def __reset_button_cb(self,widget):
        mesg = gtk.MessageDialog(type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK_CANCEL)
        mesg.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        mesg.set_markup(u"<b>Reiniciar consumo acumulado</b>")
        mesg.format_secondary_markup(MSD.MSG_RESET_CONSUM_WARNING)
        if (mesg.run() == gtk.RESPONSE_OK):
            self.conf.reset_acumulated()
            self.initial_acumulated_sent = 0
            self.initial_acumulated_recived = 0
            self.initial_acumulated_traffic = 0
            self.initial_acumulated_time = 0
            self.max_upload_speed = 0
            self.max_download_speed = 0
            self.consum_window.set_acumulated_time(0)
            self.consum_window.set_acumulated_recived(0)
            self.consum_window.set_acumulated_traffic(0)
            self.consum_window.set_acumulated_sent(0)
            self.conf.set_max_upload_speed(self.last_max_output_speed)
            self.conf.set_max_download_speed(self.last_max_input_speed)
            self.conf.save_conf()
            self.consum_window.set_max_upload_speed(self.last_max_output_speed)
            self.consum_window.set_max_download_speed(self.last_max_input_speed)

            self.notified_warning_mb = None
            self.notified_warning_percent = None
            
        mesg.destroy()
        return False

