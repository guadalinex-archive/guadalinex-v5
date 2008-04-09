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
import gtk
import gtk.gdk
import gtk.glade
import math
import cairo
import MSD
import os
import MobileManager.ui
import binascii
import re

class MSDMainStatsWidget(gtk.VBox):
	def __init__(self, mcontroller):
		gtk.VBox.__init__(self,2)

		self.mcontroller = mcontroller

		self.xml = gtk.glade.XML(os.path.join(MSD.glade_files_dir, "statswidget.glade"),
					 root="stats_eventbox")
		self.show_pcmcia_info = False
		
		self.connect_button = self.xml.get_widget("connect_button")
		self.disconnect_button = self.xml.get_widget("disconnect_button")
		self.disconnect_button.hide()
		self.consum_button = self.xml.get_widget("consum_button")
		self.stats_ebox = self.xml.get_widget("stats_eventbox")
		self.vbox_container = self.xml.get_widget("stats_vbox")
		self.connetion_label = self.xml.get_widget("connection_label")
		self.pcmcia_ebox_ext = self.xml.get_widget("pcmcia_eventbox_ext")
		self.pcmcia_ebox = self.xml.get_widget("pcmcia_eventbox")
		self.pcmcia_vbox = self.xml.get_widget("pcmcia_vbox")
		self.pcmcia_cover_image = self.xml.get_widget("pcmcia_cover_image")
		self.pcmcia_gprs_label = self.xml.get_widget("pcmcia_gprs_label")
		self.pcmcia_3g_label = self.xml.get_widget("pcmcia_3g_label")
		self.pcmcia_roaming_image = self.xml.get_widget("pcmcia_roaming_image")
		self.pcmcia_roaming_image.set_from_file(os.path.join(MSD.icons_files_dir,
									   "roaming.png"))
		self.pcmcia_x_zone_image = self.xml.get_widget("pcmcia_x_zone_image")
		self.pcmcia_roaming_image.set_no_show_all(True)
		self.pcmcia_operator_name_label = self.xml.get_widget("pcmcia_operator_name_label")
		self.time_label = self.xml.get_widget("time_label")
		self.traffic_label = self.xml.get_widget("traffic_label")
		self.speed_label =self.xml.get_widget("velocity_label")

		self.pcmcia_gprs_label.set_markup('<span foreground="grey">GPRS /</span>')
		self.pcmcia_3g_label.set_markup('<span foreground="grey">3G</span>')	
		
		self.stats_ebox.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#ecf1fb"))
		self.stats_ebox.show()
		
		self.pcmcia_ebox.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#cddcfb"))
		self.pcmcia_ebox_ext.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#4a84ff"))
		self.pcmcia_ebox_ext.show()
		
		self.pack_start(self.stats_ebox)

		self.set_cover_value(0)
		self.set_operator_name("")

		self.connection_mode = None

		#Mcontroller Signals
		self.mcontroller.connect ("active-dev-card-status-changed", self.__active_device_card_status_changed)
		self.mcontroller.connect("active-device-changed", self.__active_device_changed_cb)
		self.mcontroller.connect ("active-dev-x-zone-changed", self.__active_device_x_zone_changed)
		self.mcontroller.connect("active-dev-carrier-changed", self.__active_dev_carrier_changed_cb)
		self.mcontroller.connect("active-dev-tech-status-changed", self.__active_dev_tech_status_changed_cb)
		self.mcontroller.connect("active-dev-signal-status-changed", self.__active_dev_signal_status_changed_cb)
		self.mcontroller.connect("active-dev-roaming-status-changed", self.__active_dev_roaming_status_changed_cb)

	def __active_device_card_status_changed(self, mcontroller, status):
		if status == MobileManager.CARD_STATUS_OFF :
			self.set_operator_name("")
			self.set_all_tech_off()
			self.pcmcia_x_zone_image.hide()
		
	def __active_device_changed_cb(self, mcontroller, udi):
		self.pcmcia_x_zone_image.hide()
		self.pcmcia_roaming_image.hide()
		self.set_operator_name("")
		self.set_cover_value(0)
		self.pcmcia_x_zone_image.hide()
		
		dev = self.mcontroller.get_active_device()
		if dev == None:
			self.set_show_pcmcia_info(False)
			return

		if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
			self.set_show_pcmcia_info(False)
			return
		else:
			self.set_show_pcmcia_info(True)
			return
		
	def __active_device_x_zone_changed(self, mcontroller, zone):
		if zone == None:
			self.pcmcia_x_zone_image.hide()
		else:
			ascii_zone = binascii.a2b_hex(zone).lower()
			pattern = re.compile('.*(?P<in_zone>en zona).*')
			matched_res = pattern.match(ascii_zone)
			if matched_res != None :
				self.pcmcia_x_zone_image.set_from_file(os.path.join(MSD.icons_files_dir,
										    "xzone",
										    "home_zone.png"))
				self.pcmcia_x_zone_image.show()
				return
				
			if os.path.exists(os.path.join(MSD.icons_files_dir,"xzone","%s.png" % zone)) :
				self.pcmcia_x_zone_image.set_from_file(os.path.join(MSD.icons_files_dir,
										    "xzone",
										    "%s.png" % zone))
				self.pcmcia_x_zone_image.show()
			else:
				self.pcmcia_x_zone_image.hide()	

	def __active_dev_carrier_changed_cb(self, mcontroller, carrier_name):
		self.set_operator_name(carrier_name)

	def __active_dev_tech_status_changed_cb(self, mcontroller, tech):
		if tech == MobileManager.CARD_TECH_UMTS :
			self.set_3g_on()
		elif tech == MobileManager.CARD_TECH_HSPA :
			self.set_3g_on(hspa=True)
		else:
			self.set_gprs_on()

	def __active_dev_signal_status_changed_cb(self, mcontroller, signal):
		self.set_cover_value(signal)

	def __active_dev_roaming_status_changed_cb(self, mcontroller, roaming):
		self.set_roaming(roaming)

	def set_connection_name(self, value="Ninguna establecida"):
		self.connetion_label.set_markup('<span foreground="#003c86">%s</span>' % value)

	def set_cover_value(self, value):
		level = None
		if value == 99 :
			level = 0
		elif value < 2 :
			level = 1
		elif value < 9 :
			level = 2
		elif value < 14 :
			level = 3
		elif value < 22 :
			level = 4
		else:
			level = 5

		self.pcmcia_cover_image.set_from_file(os.path.join(MSD.icons_files_dir,
								   "cover",
								   "%s.png") % level)
	def set_gprs_on(self):
		self.pcmcia_gprs_label.set_markup('<b><span foreground="#515774"> GPRS</span></b>')
		self.pcmcia_3g_label.set_markup('')
		if self.connection_mode != "GPRS" :
			MSD.show_notification("Registrado en red GPRS", "EstÃ¡s registrado en la red GPRS.")
			self.connection_mode = "GPRS"
		
	def set_roaming(self, value):
		if value == True:
			self.pcmcia_roaming_image.show()
		else:
			self.pcmcia_roaming_image.hide()
	
	def set_3g_on(self, hspa=False):
		self.pcmcia_gprs_label.set_markup('')
		if hspa == False:
			self.pcmcia_3g_label.set_markup('<b><span foreground="#515774">3G</span></b>')
		else:
			self.pcmcia_3g_label.set_markup('<b><span foreground="#515774">3,5G</span></b>')
		if self.connection_mode != "3G" :
			MSD.show_notification("Registrado en red 3G", "EstÃ¡s registrado en la red 3G.")
			self.connection_mode = "3G"
		

	def set_all_tech_off(self):
		self.pcmcia_gprs_label.set_markup('<span foreground="grey">GPRS /</span>')
		self.pcmcia_3g_label.set_markup('<span foreground="grey">3G</span>')
		self.connection_mode = None

	def set_operator_name(self, operator_name):
		self.pcmcia_operator_name_label.set_markup('<b><span foreground="#003c86">%s</span></b>' % operator_name)

	def set_show_pcmcia_info(self,new_value):
		self.show_pcmcia_info = new_value
		if new_value:
		   self.pcmcia_ebox_ext.show_all()
		else:
		   self.pcmcia_ebox_ext.hide()


	def set_session_time(self,seconds):
		rounded_secs =  int(seconds)		
		self.time_label.set_markup('<span foreground="#003c86">%s</span>' % MSD.seconds_to_time_string(rounded_secs))
		

	def set_session_traffic(self,bytes):
		self.traffic_label.set_markup('<span foreground="#003c86">%s</span>' % MSD.format_to_maximun_unit(bytes,"GBytes","MBytes","Kbytes","bytes"))

	def set_instant_speed(self,speed):
		n_speed = (speed * 8) / 1024
		self.speed_label.set_markup('<span foreground="#003c86">%.2f Kbits/s</span>' % n_speed)


	
def main():
	window = gtk.Window()
	x = MSDMainStatsWidget()
	x.set_cover_value(0)
	x.set_gprs_on()
	x.set_operator_name("Operador")

        y = MSDMainStatsWidget()
	y.set_cover_value(0)
	y.set_gprs_on()
	y.set_operator_name("Operador")

	hbox = gtk.HBox()
	hbox.pack_start(x)
	hbox.pack_start(y)
	# Anadimos nuestro widget a la ventana
	window.add(hbox) 	
	# Conectamos el evento destroy con la salida del bucle de eventos
	window.connect("destroy", gtk.main_quit)
	# Dibujamos toda la ventana
	window.show_all()

	# Comenzamos el bucle de eventos
	gtk.main()

if __name__ == "__main__":
	main()
