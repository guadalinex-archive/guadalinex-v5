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

import gtk
import gtk.glade
import cairo
import MSD
import os

class MSDConsumWindow:
    def __init__(self, conf):
        self.conf = conf
        MSD.init_i18n()
        self.xml = gtk.glade.XML(MSD.glade_files_dir + "consum.glade")
        self.consum_window = self.xml.get_widget ("consum_window")
        self.consum_window.set_icon_from_file(MSD.icons_files_dir + "consumo_icon_16x16.png")
        self.consum_notebook = self.xml.get_widget ("consum_notebook")
        self.consum_header_image = self.xml.get_widget ("header_image")
        self.consum_header_image.set_from_file(MSD.icons_files_dir + "cabecera-consumo.png")
        self.consum_graph_hbox = self.xml.get_widget ("consum_graph_hbox")
        self.traffic_history_hbox = self.xml.get_widget ("traffic_history_hbox")
        self.consum_as_send_label = self.xml.get_widget("consum_actual_session_send_label")
        self.consum_as_recv_label = self.xml.get_widget("consum_actual_session_recv_label")
        self.consum_as_conntime_label = self.xml.get_widget("consum_actual_session_conntime_label")
        self.consum_as_mvs_label = self.xml.get_widget("consum_actual_session_mvs_label")
        self.consum_as_mvs_label.set_no_show_all(True)
        self.consum_as_mvr_label = self.xml.get_widget("consum_actual_session_mvr_label")
        self.consum_as_total_label = self.xml.get_widget("consum_actual_session_total_label")
        
        self.consum_total_sended_label = self.xml.get_widget("consum_total_sended_label")
        self.consum_total_recv_label = self.xml.get_widget("consum_total_recv_label")
        self.consum_total_traffic_label = self.xml.get_widget("consum_total_traffic_label")
        self.consum_total_traffic_label2 = self.xml.get_widget("consum_total_traffic_label2")
        self.consum_total_conntime_label = self.xml.get_widget("consum_total_conntime_label")

        self.consum_total_ro_sended_label = self.xml.get_widget("consum_total_ro_sended_label")
        self.consum_total_ro_recv_label = self.xml.get_widget("consum_total_ro_recv_label")
        self.consum_total_ro_traffic_label = self.xml.get_widget("consum_total_ro_traffic_label")
        self.consum_total_ro_traffic_label2 = self.xml.get_widget("consum_total_ro_traffic_label2")
        self.consum_total_ro_conntime_label = self.xml.get_widget("consum_total_ro_conntime_label")
        
        self.consum_max_upload_speed_label = self.xml.get_widget("consum_max_upload_speed_label")
        self.consum_max_upload_speed_label.set_no_show_all(True)
        self.consum_max_download_speed_label = self.xml.get_widget("consum_max_download_speed_label")
        self.consum_max_download_speed_label.set_no_show_all(True)
        self.consum_reset_button = self.xml.get_widget("consum_reset_button")
        self.consum_close_button = self.xml.get_widget("consum_close_button")
        self.consum_send_image = self.xml.get_widget("send_image")
        self.consum_send_image.set_from_file(MSD.icons_files_dir + "send_consum.png")
        self.consum_send_image2 = self.xml.get_widget("send_image2")
        self.consum_send_image2.set_from_file(MSD.icons_files_dir + "send_consum2.png")
        self.consum_send_image_red = self.xml.get_widget("send_image_red")
        self.consum_send_image_red.set_from_file(MSD.icons_files_dir + "send_consum2.png")
        self.consum_send_image_roaming = self.xml.get_widget("send_image_roaming")
        self.consum_send_image_roaming.set_from_file(MSD.icons_files_dir + "send_consum2.png")
        self.consum_help_button = self.xml.get_widget("consum_help_button")
        self.consum_recv_image = self.xml.get_widget("recv_image")
        self.consum_recv_image.set_from_file(MSD.icons_files_dir + "recv_consum.png")
        self.consum_recv_image2 = self.xml.get_widget("recv_image2")
        self.consum_recv_image2.set_from_file(MSD.icons_files_dir + "recv_consum2.png")
        self.consum_recv_image_red = self.xml.get_widget("recv_image_red")
        self.consum_recv_image_red.set_from_file(MSD.icons_files_dir + "recv_consum2.png")
        self.consum_recv_image_roaming = self.xml.get_widget("recv_image_roaming")
        self.consum_recv_image_roaming.set_from_file(MSD.icons_files_dir + "recv_consum2.png")
        self.header_image = self.xml.get_widget("header_image")
        self.consum_mb_warning_chk = self.xml.get_widget("consum_mb_warning_chk")
        self.consum_mb_limit_combo = self.xml.get_widget("consum_mb_limit_combo")
        self.consum_mb_limit_entry = self.xml.get_widget("consum_mb_limit_entry")

        self.consum_ro_mb_warning_chk = self.xml.get_widget("consum_ro_mb_warning_chk")
        self.consum_ro_mb_limit_entry = self.xml.get_widget("consum_ro_mb_limit_entry")
        
        self.consum_percent_warning_chk = self.xml.get_widget("consum_percent_warning_chk")
        self.consum_percent_entry = self.xml.get_widget("consum_percent_entry")
        self.consum_percent_mb_label = self.xml.get_widget("consum_percent_mb_label")
        self.consum_percent_hscale = self.xml.get_widget("consum_percent_hscale")
        self.consum_apply_button = self.xml.get_widget("consum_apply_button")
        self.consum_actual_session_type_label = self.xml.get_widget("consum_actual_session_type_label")

        self.consum_warning_image = self.xml.get_widget("consum_warning_image")
        self.consum_r_warning_image = self.xml.get_widget("consum_r_warning_image")

        self.consum_resumen_label = self.xml.get_widget("consum_resumen_label")
        self.consum_resumen_label2 = self.xml.get_widget("consum_resumen_label2")
        self.consum_clean_resumen_checkbutton = self.xml.get_widget("consum_clean_resumen_checkbutton")

        self.consum_warning_movistar_label = self.xml.get_widget("consum_warning_movistar_label")
        self.consum_warning_roaming_label = self.xml.get_widget("consum_warning_roaming_label")
                
        self.graph = MSD.MSDConsumGraph()
        self.consum_graph_hbox.pack_end(self.graph)
        
        self.history_graph = MSD.MSDTrafficHistoryGraph()
        self.traffic_history_hbox.pack_end(self.history_graph)
        
        self.traffic_hist_back = self.xml.get_widget("traffic_hist_back")
        self.traffic_hist_forward = self.xml.get_widget("traffic_hist_forward")
        self.traffic_hist_back.connect("clicked", self.__hist_go_back, None)
        self.traffic_hist_forward.connect("clicked", self.__hist_go_forward, None)
        self.traffic_hist_back.set_no_show_all(True)
        self.traffic_hist_forward.set_no_show_all(True)
		
        self.consum_window.connect("delete_event", self.__delete_event_consum_cb)
        self.consum_close_button.connect("clicked", self.__close_consum_cb, None)

        self.consum_help_button.connect("clicked", self.__show_help_online_cb, None)

    def __hist_go_back(self, widget, data):
		if self.history_graph.get_pos()>0:
			self.history_graph.set_pos(self.history_graph.get_pos()-1)
		return True
        
    def __hist_go_forward(self, widget, data):
		if self.history_graph.get_pos()<self.history_graph.get_num_columns()-self.history_graph.get_visible_columns():
			self.history_graph.set_pos(self.history_graph.get_pos()+1)
		return True
    def set_connection_type(self, ctype=None):
        if ctype == None:
            self.consum_actual_session_type_label.set_markup(_(u"<b><big>Sin Conexión</big></b>"))
        elif ctype == False:
            self.consum_actual_session_type_label.set_markup(_(u"<b><big>Red movistar</big></b>"))
        else:
            self.consum_actual_session_type_label.set_markup(_(u"<b><big>Roaming</big></b>"))
    
    
    def set_session_time(self,seconds):
        rounded_secs =  int(seconds)		
        self.consum_as_conntime_label.set_text(MSD.seconds_to_time_string(rounded_secs))

    def set_acumulated_time(self,seconds, roaming):
        rounded_secs = int(seconds)
        if roaming == False:
            self.consum_total_conntime_label.set_text(MSD.seconds_to_time_string(rounded_secs))
        else:
            self.consum_total_ro_conntime_label.set_text(MSD.seconds_to_time_string(rounded_secs))

    def set_acumulated_received(self,bytes, roaming):
        in_text = MSD.format_to_maximun_unit(bytes,"GB","MB","KB","Bytes")
        if roaming == False:
            self.consum_total_recv_label.set_text(in_text)
        else:
            self.consum_total_ro_recv_label.set_text(in_text)

    def set_acumulated_sent(self,bytes, roaming):
        in_text = MSD.format_to_maximun_unit(bytes,"GB","MB","KB","Bytes")
        if roaming == False:
            self.consum_total_sended_label.set_text(in_text)
        else:
            self.consum_total_ro_sended_label.set_text(in_text)

    def set_acumulated_traffic(self,bytes, roaming):
        in_text = MSD.format_to_maximun_unit(bytes,"GB","MB","KB","Bytes")
        if roaming == False:
            self.consum_total_traffic_label.set_text(in_text)
            self.consum_total_traffic_label2.set_text(in_text)
        else:
            self.consum_total_ro_traffic_label.set_text(in_text)
            self.consum_total_ro_traffic_label2.set_text(in_text)
  
    def set_session_traffic(self,received_bytes,sent_bytes):
        in_text = MSD.format_to_maximun_unit(received_bytes,"GB","MB","KB","Bytes")
        out_text = MSD.format_to_maximun_unit(sent_bytes,"GB","MB","KB","Bytes")
        total_text = MSD.format_to_maximun_unit((sent_bytes+received_bytes), "GB","MB","KB","Bytes")
        self.consum_as_recv_label.set_text(in_text) 
        self.consum_as_send_label.set_text(out_text)
        self.consum_as_total_label.set_text(total_text)
    
    def set_as_send_value(self, value):
        self.consum_as_send_label.set_text(value)

    def set_as_recv_value(self, value):
        self.consum_as_recv_label.set_text(value)

    

    def set_as_mvs_value(self, value):
        self.consum_as_mvs_label.set_text(value)

    def set_as_mvr_value(self, value):
        self.consum_as_mvr_label.set_markup("<b>%s</b>" % value)

    def set_total_sended_value(self, value):
        self.consum_total_sended_label.set_text(value)

    def set_total_recv_value(self, value):
        self.consum_total_recv_label.set_text(value)

    def set_total_conntime_value(self, value):
        self.consum_total_conntime_label.set_text(value)

    def show_all(self):
        self.consum_percent_hscale.set_value (self.conf.get_warning_percent())
        self.consum_percent_entry.set_value (self.conf.get_warning_percent())
        if self.conf.get_warning_mb_combo() == 3 :
            self.consum_mb_limit_entry.set_value (self.conf.get_warning_mb(False))
            self.consum_mb_limit_entry.set_sensitive(True)
        else:
            self.consum_mb_limit_entry.set_sensitive(False)
        
        tmp_values = {
            0: 200,
            1: 1024,
            2: (5 * 1024),
            3: self.conf.get_warning_mb(False)
            }
        
        warning_mb = tmp_values.get(self.conf.get_warning_mb_combo())
        warning_mb_percent = (warning_mb * self.conf.get_warning_percent()) / 100
        self.consum_percent_mb_label.set_text ("")
        self.consum_percent_mb_label.hide()
        
        self.consum_mb_limit_combo.set_active (self.conf.get_warning_mb_combo())
        self.consum_mb_warning_chk.set_active (self.conf.get_show_mb_warning(False))
        self.consum_ro_mb_warning_chk.set_active (self.conf.get_show_mb_warning(True))
        self.consum_percent_warning_chk.set_active (self.conf.get_show_percent_warning())
        
        self.consum_apply_button.set_sensitive(False)
        self.consum_window.show_all()
        self.consum_window.deiconify()
        self.consum_notebook.set_current_page(0)
        self.consum_close_button.grab_focus()

    def __close_consum_cb(self, widget, data):
        self.consum_window.hide_all()

    def __delete_event_consum_cb(self, widget, data):
        self.__close_consum_cb(widget, data)
        return True

    def __show_help_online_cb(self, widget, data):
        os.system("gnome-open http://www.canalcliente.movistar.es")


    def set_maximun_output_speed(self,speed):
        n_speed = (speed * 8) / 1024
        self.consum_as_mvs_label.set_text(_(u"%.2f Kbits/s") % n_speed)

    def set_maximun_input_speed(self,speed):
        n_speed = (speed * 8) / 1024
        if n_speed > 10000 :
            return
        self.consum_as_mvr_label.set_markup(_(u"<b>%.2f Kbits/s</b>") % n_speed)

    def set_max_upload_speed(self,speed):
        n_speed = (speed * 8) / 1024
        self.consum_max_upload_speed_label.set_text(_(u"%.2f Kbits/s") % n_speed)

    def set_max_download_speed(self,speed):
        n_speed = (speed * 8) / 1024
        if n_speed > 10000 :
            return
        self.consum_max_download_speed_label.set_text(_(u"%.2f Kbits/s") % n_speed)
