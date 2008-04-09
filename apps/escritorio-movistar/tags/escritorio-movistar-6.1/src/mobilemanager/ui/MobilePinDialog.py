#!/usr/bin/python
# -*- coding: iso-8859-15 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica Móviles España S.A.U.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#
import MobileManager.ui
import gtk
import gtk.glade
import os
import time

def is_valid_pin(pin):
	try:
		int(pin)
        except:
		return False
        
        if len(pin) >3 and len(pin) <9:
		return True
        else:
		return False

class MobileAskPinDialog:
	
	def __init__(self, mcontroller):
		main_ui_filename = os.path.join(MobileManager.ui.mobilemanager_glade_path, "mm_pin_dialog.glade")
		self.mcontroller = mcontroller
		widget_tree = gtk.glade.XML(main_ui_filename,"ask_pin_dialog")
		self.dialog = widget_tree.get_widget("ask_pin_dialog")
		self.pin_entry = widget_tree.get_widget("pin_entry")
		self.pin_error_label = widget_tree.get_widget("pin_error_label")
		self.error_hbox =  widget_tree.get_widget("error_hbox")
		self.ok_button = widget_tree.get_widget("ok_button")
		self.ok_button.set_sensitive(False)
		
		self.pin_entry.connect("changed", self.pin_entry_changed_cb, None)

	def pin_entry_changed_cb(self, editable, data):
		if len(self.pin_entry.get_text()) > 0:
			self.ok_button.set_sensitive(True)
		else:
			self.ok_button.set_sensitive(False)
        
	def run(self):
		bucle = 0
		dev = self.mcontroller.get_active_device()

		if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
			return

		status = dev.pin_status()
		
		if status != MobileManager.PIN_STATUS_WAITING_PIN :
			return

		self.pin_entry.set_text("")
		self.error_hbox.hide()
		self.dialog.show()

		
		while status ==  MobileManager.PIN_STATUS_WAITING_PIN :
			
			response = self.dialog.run()
			bucle = bucle + 1 
			print "ASK PIN WHILE %s" % bucle
			
			if response != gtk.RESPONSE_OK:           
				dlg = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_ERROR, gtk.BUTTONS_OK, _("You cancel the PIN code insertion process, the mobile device will be turn off"))
				dlg.set_title(_("PIN Code insertion canceled"))
				dlg.run()
				dlg.destroy()
				self.dialog.hide()
				try:
					dev.turn_off()
				except Exception,msg:
					print "ERROR apagando tarjeta %s" % msg
				return
			
			pin = self.pin_entry.get_text()

			if not is_valid_pin(pin):
				self.pin_error_label.set_markup('<b>%s</b>' % _("The PIN Code require between 4 and 8 digits"))                
				self.error_hbox.show_all()
				continue
            
			try:
				res = dev.send_pin(pin)
				time.sleep(2)
				status = dev.pin_status()

				if status == MobileManager.PIN_STATUS_WAITING_PUK:
					self.dialog.hide()
					ask_puk = MobileManager.ui.MobilePukDialog(self.mcontroller)
					return ask_puk.run()
				
				if res == True and status != MobileManager.PIN_STATUS_WAITING_PIN:
					self.dialog.hide()
					return
				
				self.pin_error_label.set_markup('<b>%s.</b>' % _("The PIN Code is not valid"))
				self.pin_entry.set_text("")
				self.error_hbox.show_all()
			except Exception,msg:
				print "Error send pin %s" % msg
				self.pin_error_label.set_markup('<b>%s</b>' % _("There is a problem with you Mobile Device. The application can not communicate with it"))
				self.error_hbox.show_all()
				return 


class  MobileChangePinDialog:

	def __init__(self,mcontroller):
		self.mcontroller = mcontroller
		main_ui_filename = os.path.join(MobileManager.ui.mobilemanager_glade_path,"mm_pin_dialog.glade")
		widget_tree = gtk.glade.XML(main_ui_filename,"change_pin_dialog")
		self.dialog = widget_tree.get_widget("change_pin_dialog")
		self.error_label = widget_tree.get_widget("change_pin_error_label")
		self.error_hbox = widget_tree.get_widget("change_pin_error_hbox")
		self.current_pin_entry  = widget_tree.get_widget("current_pin_entry")
		self.new_pin_entry  = widget_tree.get_widget("new_pin_entry")
		self.new_pin_confirm_entry  = widget_tree.get_widget("new_pin_confirm_entry")
		self.ok_button =  widget_tree.get_widget("ok_button")
		self.ok_button.set_sensitive(False)
		
		self.new_pin_entry.connect("changed", self.entries_changed_cb, None)
		self.new_pin_confirm_entry.connect("changed", self.entries_changed_cb, None)
		self.current_pin_entry.connect("changed", self.entries_changed_cb, None)
	
	def entries_changed_cb(self, editable, data):
		if len(self.current_pin_entry.get_text()) > 0 and len(self.new_pin_entry.get_text()) > 0 and len(self.new_pin_confirm_entry.get_text()) > 0:
			self.ok_button.set_sensitive(True)
		else:
			self.ok_button.set_sensitive(False)

	def run(self):
		dev = self.mcontroller.get_active_device()

		if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
			return

		status = dev.pin_status()
		if not (status == MobileManager.PIN_STATUS_READY or status == MobileManager.PIN_STATUS_WAITING_PUK) :
			return 

		stop = False
		
		self.current_pin_entry.set_text("")
		self.new_pin_entry.set_text("")
		self.new_pin_confirm_entry.set_text("")
		self.error_label.show()
		self.error_hbox.hide()
		self.dialog.show()
		
		while not stop :
			response = self.dialog.run()
			
			if response != gtk.RESPONSE_OK:
				self.dialog.hide()
				return

			new_pin = self.new_pin_entry.get_text()
			new_pin_confirm = self.new_pin_confirm_entry.get_text()
			current_pin = self.current_pin_entry.get_text()

			if not is_valid_pin(new_pin) or not is_valid_pin(current_pin):
				self.error_label.set_markup('<b>%s</b>' % _("The PIN Code require between 4 and 8 digits"))
				print "1"
				self.error_hbox.show_all()
				continue
            
			if new_pin != new_pin_confirm:
				self.error_label.set_markup('<b>%s</b>' % _("The two PIN code are not equal"))
				print "2"
				self.error_hbox.show_all()
				continue
            
			try:
				res = dev.set_pin(current_pin,new_pin)
				time.sleep(2)
				status = dev.pin_status()
				if status == MobileManager.PIN_STATUS_WAITING_PUK:
					dev.turn_off()
					self.dialog.hide()
					dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
					dlg.set_markup(_("<b>Error changing PIN Code</b>"))
					dlg.format_secondary_markup(_("The PIN code changing process of your SIM card has failed. Now , your card will be turn off. The next turn on of your card, the PUK Code will be required"))
					dlg.run()
					dlg.destroy()
					return
				
				if res == True :
					self.dialog.hide()
					return
				
				self.error_label.set_markup('<b>%s.</b>' % _("The PIN Code is not valid"))
				self.current_pin_entry.set_text("")
				self.error_hbox.show_all()
				
			except Exception,msg:
				self.error_label.set_markup('<b>%s</b>' %  _("There is a problem with you Mobile Device. The application can not communicate with it"))
				self.error_hbox.show_all()
				print "Error change pin: %s" %msg
				return


class MobileManagePinDialog:
	
	def __init__(self, mcontroller):        
		self.mcontroller = mcontroller
		main_ui_filename = os.path.join(MobileManager.ui.mobilemanager_glade_path,"mm_pin_dialog.glade")
		widget_tree = gtk.glade.XML(main_ui_filename,"manage_pin_dialog")
		self.dialog = widget_tree.get_widget("manage_pin_dialog")
		self.header_info_label = widget_tree.get_widget("header_info_label")
		self.info_label = widget_tree.get_widget("info_label")
		self.error_label = widget_tree.get_widget("manage_pin_error_label")
		self.error_hbox = widget_tree.get_widget("manage_pin_error_hbox")
		self.pin_entry  = widget_tree.get_widget("pin_entry")
		self.ok_button = widget_tree.get_widget("ok_button")
		self.ok_button.set_sensitive(False)
		self.pin_entry.connect("changed", self.pin_entry_changed_cb, None)
	
	def pin_entry_changed_cb(self, editable, data):
		if len(self.pin_entry.get_text()) > 0:
			self.ok_button.set_sensitive(True)
		else:
			self.ok_button.set_sensitive(False)

	def run_activate(self):
		self.run(True)

	def run_deactivate(self):
		self.run(False)
		
	def run(self,activate):
		if activate == True :
			self.info_label.set_markup (_("The <b>PIN Code</b> of your SIM card is not active. If you want activate it, introduce your <b>PIN Code</b> and press accept button.\nYou have 3 attempts to introduce the PIN code correctly. If you fail, the card will be blocked."))
			self.header_info_label.set_markup(_("<b><big>Activate Pin Code</big></b>"))
			self.dialog.set_title(_("Activate Pin Code"))
		else:
			self.info_label.set_markup (_("The <b>PIN Code</b> of your SIM card is active. If you want deactivate it, introduce your <b>PIN Code</b> and press accept button.\nYou have 3 attempts to introduce the PIN code correctly. If you fail, the card will be blocked."))
			self.header_info_label.set_markup(_("<b><big>Deactivate Pin Code</big></b>"))
			self.dialog.set_title(_("Deactivate Pin Code"))
		
		self.error_label.set_text("")
		self.error_label.hide()

		dev = self.mcontroller.get_active_device()
		print dev

		if not MobileManager.AT_COMM_CAPABILITY in dev.capabilities :
			return

		status = dev.pin_status()
		if status != MobileManager.PIN_STATUS_READY :
			return

		if dev.is_pin_active() == activate :
			return
        
		while True:
			self.pin_entry.set_text("")
			response = self.dialog.run()
			
			if response != gtk.RESPONSE_OK:
				print "Return"
				self.dialog.hide()
				return
			
			pin = self.pin_entry.get_text()
            
			if not is_valid_pin(pin):
				self.error_label.set_markup('<b>%s</b>' % _("The PIN Code require between 4 and 8 digits"))
				self.error_hbox.show_all()
				continue
			
			try:
				res = dev.set_pin_active(pin, activate)
				time.sleep(2)
				status = dev.pin_status()
				
				if status == MobileManager.PIN_STATUS_WAITING_PUK:
					dev.turn_off()
					self.dialog.hide()
					dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
					dlg.set_markup(_("<b>Wrong insertion of PIN Code</b>"))
					dlg.format_secondary_markup(_("The PIN code insertion process of your SIM card has failed. Now , your card will be turn off. The next turn on of your card, the PUK Code will be required"))
					dlg.run()
					dlg.destroy()
					return

				if res == True :
					self.dialog.hide()
					return
				
				self.error_label.set_markup('<b>%s.</b>' % _("The PIN Code is not valid"))
				self.pin_entry.set_text("")
				self.error_hbox.show_all()
				
			except Exception, msg:
				self.error_label.set_markup('<b>%s</b>' % _("There is a problem with you Mobile Device. The application can not communicate with it"))
				self.error_hbox.hide()
				print "Error : %s" %msg
