#!/usr/bin/python
# -*- coding: utf-8 -*-
#
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#           Cesar Garcia <cesar.garcia@openshine.com>
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
import re
import csv
import pygtk
import pango
import gnome
pygtk.require("2.0")
import gtk
import gtk.glade
import gtk.gdk
import gobject
from backend import get_user_addressbook ,MDContact
import emsync
import time
import MobileManager 
MIN_SEARCH_LEN = 0

class MDImportProgress:
	def __init__(self, sync):
		MSD.init_i18n()
		self._widget_tree = gtk.glade.XML(MSD.glade_files_dir + "agenda.glade" ,"import_progress_dialog")
		self._dlg = self._widget_tree.get_widget("import_progress_dialog")
		self._progress = self._widget_tree.get_widget("progressbar")
		self._cancel = self._widget_tree.get_widget("cancel_button")
		self._cancel.connect("clicked",self.on_clicked)
		self._dlg.connect("destroy",self.on_destroy)
		self._image = self._widget_tree.get_widget("image")
		self._anim = gtk.gdk.PixbufAnimation(MSD.icons_files_dir + "movistar.gif")
		self._animit = self._anim.get_iter()
		self._timeout = gobject.timeout_add(100, self.on_timeout)
		self._timeoutanim = gobject.timeout_add(50, self.on_timeout_anim)
		self._sync = sync
		self._cancelando = False
		self._dlg.set_deletable(False)
		self._dlg.set_icon_from_file(MSD.icons_files_dir + "synchronize_32x32.png")
		self._dlg.run()

	def on_clicked(self,widget,event=None):
		print "clicked"
		if self._sync.running():
			self._cancelando = True
		else:
			if self._timeout is not None:
				gobject.source_remove(self._timeout)
			if self._timeoutanim is not None:
				gobject.source_remove(self._timeoutanim)			
			self._timeout=None
			self._timeoutanim=None
			self._dlg.destroy()

	def on_destroy(self,widget,event=None):
		print "destroy"
		if self._timeout is not None:
			gobject.source_remove(self._timeout)
		if self._timeoutanim is not None:
			gobject.source_remove(self._timeoutanim)			
		self._timeout=None
		self._timeoutanim=None

	def on_timeout_anim(self):
		if not self._sync.running():
			self._animit = self._anim.get_iter()
			self._animit.advance(time.time()+1.)
			self._image.set_from_pixbuf(self._animit.get_pixbuf())
			self.timeoutanim=None
			return False
		else:
			if self._animit.advance():
				self._image.set_from_pixbuf(self._animit.get_pixbuf())
		return True
	
	def on_timeout(self):
		if self._sync is not None:		
			progress = self._sync.get_progress()
			self._progress.set_fraction(progress/100.0)
			
			statex = self._widget_tree.get_widget("state")
			
			state = self._sync.get_state()
			running = self._sync.running()
			
			if running and self._cancelando:
				statex.set_text("Cancelando...")
				self._sync.request_shutdown()
				self._cancel.set_sensitive(False)
				self._cancel.queue_resize()
				self._cancel.queue_draw()
				# self._cancelando=False				
			elif state==self._sync.State_Connecting:
				statex.set_text(_(u"Conectando..."))
			elif state==self._sync.State_InitSync:
				statex.set_text(_(u"Iniciando importación..."))
			elif state==self._sync.State_Downloading:
				statex.set_text(_(u"Descargando..."))
			elif state==self._sync.State_Acknowledge:
				statex.set_text(_(u"Confirmando datos..."))
			elif state==self._sync.State_Cancelled:
				statex.set_text(_(u"Cancelado."))
			elif state==self._sync.State_Finished:
				statex.set_text(_(u"Importación correcta."))
			elif state==self._sync.State_FinishedWithError:
				statex.set_text(_(u"Se produjo un error al importar."))
			elif state==self._sync.State_FinishedWithPartialDownload:
				statex.set_text(_(u"No se importaron todos los datos."))
			statex.queue_resize()
			statex.queue_draw()
			
			if not running:
				self._cancel.set_sensitive(True)
				self._cancel.set_label("Cerrar")
				self._cancel.queue_resize() # si no, no se repinta
				self._cancel.queue_draw()				
				error = self._sync.get_error()
				if self._cancelando:
					statex.set_text(_(u"Cancelado."))
				elif error==self._sync.Error_NoDB:
					statex.set_text(_(u"No se ha podido abrir la agenda."))
				elif error==self._sync.Error_CouldNotConnect:
					statex.set_text(_(u"No se ha podido conectar."))
				elif error==self._sync.Error_InvalidReply:
					statex.set_text(_(u"Se ha recibido una respuesta incorrecta."))
				elif error==self._sync.Error_BadAuth:
					statex.set_text(_(u"El usuario/contraseña son incorrectos."))
				elif error==self._sync.Error_Unknown:
					statex.set_text(_(u"Error desconocido."))
				self.timeout=None
				return False
				
		return True

class MDContactEditor:

    def __init__(self):
	MSD.init_i18n()
        self._widget_tree = gtk.glade.XML(MSD.glade_files_dir + "agenda.glade" ,"contact_editor_dialog")
        self._dlg = self._widget_tree.get_widget("contact_editor_dialog")
        self._dlg.set_icon_from_file(MSD.icons_files_dir + "addressbook_16x16.png")
        self._dlg.hide()
        self._dlg.connect("delete-event",self.on_dialog_delete)
        self._name_entry = self._widget_tree.get_widget("name_entry")
        self._mobile_phone_entry = self._widget_tree.get_widget("mobile_phone_entry")
        self._email_entry = self._widget_tree.get_widget("email_entry")
        self._ok_button = self._widget_tree.get_widget("ok_button")
        
        self._ok_button.set_sensitive(False)

        self._name_entry.connect("changed", self.__entry_changed, None)
        self._mobile_phone_entry.connect("changed", self.__entry_changed, None)
        self._email_entry.connect("changed", self.__entry_changed, None)

    def __is_min_entries_empty (self):
        result = False
        if self._name_entry.get_text() == "":
            result = True
        if self._mobile_phone_entry.get_text() == "":
            result = True

        return result

    def __entry_changed (self, widget, data):
        if self.__is_min_entries_empty():
            self._ok_button.set_sensitive(False)
        else:
            self._ok_button.set_sensitive(True)

    def get_dict(self):
        d =  {}
        d["name"]  = self._name_entry.get_text()
        d["phone"] = self._mobile_phone_entry.get_text()
        d["email"] = self._email_entry.get_text()
        d["copia_agenda_id"] = ""
        d["modification_stringdate"] = ""
        return d
    
    def run(self,contact=None):
        if contact is None:
            self._dlg.set_title(_(u"Nuevo contacto"))
            self.__clear_fields()
        else:
            self._dlg.set_title(_(u"Modificar contacto"))
            self.__populate_fields(contact)

	while True:
            self._name_entry.grab_focus()
            response = self._dlg.run()

	    if (response == gtk.RESPONSE_OK):
                p = re.compile("^[0-9\+\#\*_]*$")
                print self._mobile_phone_entry.get_text()
                if p.match (self._mobile_phone_entry.get_text()) == None:
                    mesg = gtk.MessageDialog(None,
                                             gtk.DIALOG_MODAL,
                                             gtk.MESSAGE_ERROR,
			                     gtk.BUTTONS_CLOSE)
                    mesg.set_icon_from_file(MSD.icons_files_dir + "addressbook_16x16.png")
		    mesg.set_markup(_(u"<b>Número de teléfono incorrecto</b>"))
		    mesg.format_secondary_markup(_(u"Número de teléfono incorrecto. Este campo sólo permite números y los caracteres +, # y *."))

                    if (mesg.run() != None):
                        mesg.destroy()
		    continue
		else:
		    break
	    else:
	        break

        self._dlg.response(response)
        self._dlg.hide()
        return response
    
    def on_dialog_delete(self,widget,event=None):
        return True

    def __populate_fields(self,contact):
        self._name_entry.set_text(contact.name)
        self._mobile_phone_entry.set_text(contact.phone)
        self._email_entry.set_text(contact.email)
        
    def __clear_fields(self):
        self._name_entry.set_text("")
        self._mobile_phone_entry.set_text("")
        self._email_entry.set_text("")
        
class MDAddressBookController:

    def __init__(self,address_book,conf,preferences):
	MSD.init_i18n()
        self._addressbook = address_book
        self.conf = conf
        self.preferences = preferences
        self._model = None
        self._last_search_text =""
        self._clip_board = None

        # Main window
        self._widget_tree = gtk.glade.XML(MSD.glade_files_dir + "agenda.glade", "main_window")
        self._main_window = self._widget_tree.get_widget("main_window")
        self._header_image = self._widget_tree.get_widget("header_image")

        # add
        self._add_contact_button = self._widget_tree.get_widget("add_contact_button")
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "contactonuevo_32x32.png")
        self._add_contact_button.set_icon_widget(icon)
        icon.show()

        # remove
        self._remove_contact_button = self._widget_tree.get_widget("remove_contact_button")
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "eliminarcontacto_32x32.png")
        self._remove_contact_button.set_icon_widget(icon)
        icon.show()

        # synchronize
        self._synchronize_contacts_button = self._widget_tree.get_widget("synchronize_contacts_button")
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "synchronize_32x32.png")
        self._synchronize_contacts_button.set_icon_widget(icon)
        icon.show()

        # edit
        self._edit_contact_button = self._widget_tree.get_widget("edit_contact_button")
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "editarcontacto_32x32.png")
        self._edit_contact_button.set_icon_widget(icon)
        icon.show()
        
        self._send_mail_button = self._widget_tree.get_widget("mail_button")
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "mail_32x32.png")
        self._send_mail_button.set_icon_widget(icon)
        icon.show()
        
        self._contact_treeview = self._widget_tree.get_widget("contact_treeview")
        self._search_entry = self._widget_tree.get_widget("search_entry")
        self._clear_search_entry_button = self._widget_tree.get_widget("clear_search_entry_button")
        tmp_widget = self._widget_tree.get_widget("search_image")
        tmp_widget.set_from_file(MSD.icons_files_dir + "buscar_16x16.png")

        # Looking
        self._main_window.set_icon_from_file(MSD.icons_files_dir + "addressbook_16x16.png")
        self._header_image.set_from_file(os.path.join(MSD.icons_files_dir,"cabecera-agenda.png"))

        # About dialog
        self._xml_about = gtk.glade.XML(MSD.glade_files_dir + "agenda.glade", "about_dialog")
        self._about_dialog = self._xml_about.get_widget ("about_dialog")
        self._about_dialog.set_name (_(u"Escritorio Movistar - Beta"))
        self._about_dialog.set_version (self.conf.get_version())
        self._about_dialog.set_logo (gtk.gdk.pixbuf_new_from_file((MSD.icons_files_dir + "addressbook_48x48.png")))
        self._about_dialog.set_icon_from_file(MSD.icons_files_dir + "addressbook_16x16.png")
        self._about_dialog.hide()
        self._about_dialog.connect("delete-event", self.__on_about_close_clicked)

        # Menu
        self._menu_modif_contact_item = self._widget_tree.get_widget("modificar_contacto1")
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "editarcontacto_16x16.png")
        self._menu_modif_contact_item.set_image(icon)
        self._menu_remove_contact_item = self._widget_tree.get_widget("borrar_contacto1")
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "eliminarcontacto_16x16.png")
        self._menu_remove_contact_item.set_image(icon)
        menu_item = self._widget_tree.get_widget("nuevo_contacto1")
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "contactonuevo_16x16.png")
        menu_item.set_image(icon)
        menu_item2 = self._widget_tree.get_widget("importar1")
        icon2 = gtk.Image() ; icon2.set_from_file(MSD.icons_files_dir + "synchronize_16x16.png")
        menu_item2.set_image(icon2)
        item_dict = {"on_nuevo_contacto1_activate": self.on_add_button_clicked,
                     "on_exportar1_activate": self.__on_export_clicked,
                     "on_importar1_activate": self.__on_import_clicked,
                     "on_salir1_activate": self.on_window_delete,
                     #"on_cortar1_activate": self.__on_cut_clicked,
                     #"on_copiar1_activate": self.__on_copy_clicked,
                     #"on_pegar1_activate": self.__on_paste_clicked,
                     "on_modificar_contacto1_activate": self.on_edit_button_clicked,
                     "on_borrar_contacto1_activate": self.on_remove_button_clicked,
		     "on_importar1_activate": self.on_synchronize_button_clicked,
                     "on_abrir_configuracion1_activate": self.__on_config_clicked,
                     "on_ver_ayuda1_activate": self.__on_show_help_clicked
                     #"on_acerca_de1_activate": self.__on_about_clicked
                     }
        self._widget_tree.signal_autoconnect(item_dict)

        # Contacts treeview
        self.__build_model()
        self._contact_editor = MDContactEditor()
        self.__build_treeview(self._contact_treeview)
        tips = gtk.Tooltips()
        tips.set_tip(self._contact_treeview, "")

        # Contextual menu
        self._xml = gtk.glade.XML(MSD.glade_files_dir + "agenda.glade", "addresbook_ctx_menu")
        self._contextual_menu = self._xml.get_widget ("addresbook_ctx_menu")
        
        menu_item = gtk.ImageMenuItem (_(u"Eliminar contacto"))
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "eliminarcontacto_16x16.png")
        menu_item.set_image(icon)
        menu_item.connect ("activate", self.on_remove_button_clicked)
        menu_item.show_all()
        self._contextual_menu.append (menu_item)
        
        menu_item = gtk.ImageMenuItem (_(u"Modificar contacto"))
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "editarcontacto_16x16.png")
        menu_item.set_image(icon)
        menu_item.connect ("activate", self.on_edit_button_clicked)
        menu_item.show_all()
        self._ctx_edit_contact__menu_item = menu_item
        self._contextual_menu.append (menu_item)
        
        
        menu_item = gtk.ImageMenuItem (_(u"Nuevo contacto"))
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "contactonuevo_16x16.png")
        menu_item.set_image(icon)
        menu_item.connect ("activate", self.on_add_button_clicked)
        menu_item.show_all()
        self._contextual_menu.append (menu_item)
        
        menu_item = gtk.ImageMenuItem(_(u"Enviar Correo"))
        icon = gtk.Image() ; icon.set_from_file(MSD.icons_files_dir + "mail_16x16.png")
        menu_item.set_image(icon)
        menu_item.connect ("activate", self.on_send_mail_button_clicked)
        menu_item.show_all()
        self._ctx_send_mail_menu_item = menu_item
        self._contextual_menu.prepend (menu_item)

        # Signals
        self._main_window.connect("delete-event",self.on_window_delete)
        self._add_contact_button.connect("clicked",self.on_add_button_clicked)
        self._remove_contact_button.connect("clicked",self.on_remove_button_clicked)
        self._edit_contact_button.connect("clicked",self.on_edit_button_clicked)
        self._synchronize_contacts_button.connect("clicked",self.on_synchronize_button_clicked)
        self._send_mail_button.connect("clicked",self.on_send_mail_button_clicked)
        self._search_entry.connect("changed",self.on_search_entry_changed);
        self._clear_search_entry_button.connect("clicked", self.on_clear_search_entry_button_clicked)
        
        self._contact_treeview.connect("button_press_event",self.__contact_treeview_button_event_cb)
        self._contact_treeview.connect("key_press_event",self.__contact_treeview_key_event_cb)
        self._contact_treeview.connect("cursor-changed",self.__contact_treeview_cursor_changed_cb,None)
        self._contact_treeview.connect("motion-notify-event", self.__contact_treeview_tooltip, tips, None)
        treeselection = self._contact_treeview.get_selection()
	treeselection.set_mode (gtk.SELECTION_MULTIPLE)
        treeselection.connect ("changed", self.__contact_treeview_cursor_changed_cb, None)

        # Glade fix
        #tmp_widget = self._widget_tree.get_widget ("addb_alignment_search")
        #tmp_widget.set (1, 0.50, 0, 1)

        self._main_window.hide()

    def __contact_treeview_tooltip(self,  widget, e, tooltips, cell, emptyText=""):
        try:
            (path,col,x,y) = widget.get_path_at_pos(int(e.x), int(e.y))
        except:
            tooltips.disable()
            return

        it = widget.get_model().get_iter(path)
        value = "%s\n%s\n%s" % (widget.get_model().get_value(it, 1),
                                widget.get_model().get_value(it, 2),
                                widget.get_model().get_value(it, 3))
        tooltips.set_tip(widget, value)
        tooltips.enable()

    def __build_treeview(self,tree_view):
        db_fields =   ["name","phone","email" ]

        base_id = 1
        self._columns = []        
        
        for field in [_(u"Nombre"),_(u"Teléfono"),_(u"Correo electrónico")]:
            col = gtk.TreeViewColumn(field)
            tree_view.append_column(col)
            cell = gtk.CellRendererText()
            col.pack_start(cell, True)
            col.add_attribute(cell, 'text', base_id)
            col.set_sort_column_id(base_id)
            col.set_cell_data_func(cell,self.__cell_render_func,self._search_entry)
            col.set_resizable(True)
            col.set_reorderable(True)
            cell.set_property("editable", True)
            cell.set_property("ellipsize", pango.ELLIPSIZE_END)
            cell.connect("edited", self.__cell_edited_cb,(base_id,db_fields[base_id -1]))
            event_box = gtk.EventBox()
            event_box.add(gtk.Label(field))
            col.set_widget(event_box)
            tt = gtk.Tooltips()
            tt.set_tip (event_box, field)
            event_box.show_all()
            self._columns.append(col)
            base_id = base_id + 1
            # Hack for tool tip

        self._contact_treeview.set_cursor(0)

    def __cell_edited_cb(self,cell, path, new_text, field_index_db_field_tuple=None):
        field_index, db_field = field_index_db_field_tuple
        if new_text == "":
            return
        
        obj_id = self._model[path][0]
        value =  self._model[path][field_index]
        contact = MDContact.get(obj_id)
        d ={db_field : new_text}
        d["copia_agenda_id"]=""
        contact.set(**d)
        self.__build_model()
             
    def __cell_render_func(self,column, cell_renderer, model, iter, search_entry):
        search_text = search_entry.get_text().strip().lower()
        if len(search_text) <= MIN_SEARCH_LEN :
             return

        idx =  self._contact_treeview.get_columns().index(column) + 1
        value = model.get_value(iter, idx).lower()
        idx = value.find(search_text)
        if idx >=0:
            prefix = value[:idx]
            body = value[idx: idx +len(search_text)]
            suffix = value[idx +len(search_text):]
            cell_renderer.set_property('markup','%s<span background="blue" foreground="white">%s</span>%s' % (prefix,body,suffix))
            
    def __search_filter_func(self,model,iter,search_entry):
        search_text = search_entry.get_text()
        if len(search_text) <= MIN_SEARCH_LEN:
            return True
        
        num_col = model.get_n_columns()
        for i in range(num_col -1):
            value = model.get_value(iter,i+1).strip().lower()
            if  search_text.lower() in value:
                return True
            
        return False
    
    def __build_model(self):
        old_model = self._model        
        model = gtk.ListStore(gobject.TYPE_INT,
                               gobject.TYPE_STRING,
                               gobject.TYPE_STRING,
                               gobject.TYPE_STRING)
        for contact in  self._addressbook.get_all_contacts():
            model.append([contact.id ,
                           contact.name,
                           contact.phone,
                           contact.email])

        search_text = self._search_entry.get_text().strip()

        model = model.filter_new()
        model.set_visible_func(self.__search_filter_func,self._search_entry)
        
        model = gtk.TreeModelSort(model)
        self._model =  model
         
        if old_model is None:
            self._model.set_sort_column_id(1, gtk.SORT_ASCENDING)
        else:
            col_id, ordering = old_model.get_sort_column_id()
            if col_id is not None:
                self._model.set_sort_column_id(col_id, ordering)
   
        self._contact_treeview.set_model(self._model)
       
    def __selected_contacts(self):
        contacts = []
        selection = self._contact_treeview.get_selection()
	selection.selected_foreach (self.__contact_selection_foreach_cb, contacts)
	return contacts

    def __contact_selection_foreach_cb (self, model, path, iter, contacts):
        contact_id = model.get_value(iter,0)
        target_contact  = MDContact.get(contact_id)
	contacts.append (target_contact)
    
    def __contact_treeview_button_event_cb(self,widget,event):
        if event.type == gtk.gdk._2BUTTON_PRESS:
            self.on_edit_button_clicked(widget)
            
        if event.button == 3:
            time = event.time

            path = self._contact_treeview.get_path_at_pos(int(event.x),int(event.y))
            selection = self._contact_treeview.get_selection()
	    rows = selection.get_selected_rows()
	    
	    if path[0] not in rows[1]:
	        selection.unselect_all()
		selection.select_path(path[0])

	    self._contextual_menu.popup (None, None, None, event.button, time)
		
            return True

    def __contact_treeview_key_event_cb (self, widget, event):
         if (event.keyval == gtk.gdk.keyval_from_name ('Delete')):
             self.on_remove_button_clicked(self,widget)
         return False


    def __contact_treeview_cursor_changed_cb(self,widget,event):
        self.__check_button_sensitivity()

    def __check_button_sensitivity(self):
        selected_rows =  self._contact_treeview.get_selection().count_selected_rows()
        buttons = (self._edit_contact_button,self._remove_contact_button,self._send_mail_button, self._menu_modif_contact_item, self._menu_remove_contact_item)
        new_state = selected_rows > 0
        for button in buttons:
            button.set_sensitive(new_state)

        # Check email selected
        if selected_rows > 0:
	    active_emails = False
	    contacts = self.__selected_contacts()
	    for contact in contacts:
	        if contact.email != "":
		    active_emails = True
		    break

            if active_emails:
                self._send_mail_button.set_sensitive (True)
                self._ctx_send_mail_menu_item.set_sensitive (True)
            else:
                self._send_mail_button.set_sensitive (False)
                self._ctx_send_mail_menu_item.set_sensitive (False)

	# Check edit button sensitivity
	if selected_rows == 0 or selected_rows > 1:
            self._edit_contact_button.set_sensitive (False)
            self._ctx_edit_contact__menu_item.set_sensitive (False)
            self._menu_modif_contact_item.set_sensitive (False)
	else:
            self._edit_contact_button.set_sensitive (True)
            self._ctx_edit_contact__menu_item.set_sensitive (True)
            self._menu_modif_contact_item.set_sensitive (True)

    def __on_export_clicked(self,widget,event=None):
        dialog = gtk.FileChooserDialog (_(u"Seleccione el fichero destino para exportar"),
                                        self._main_window,
                                        gtk.FILE_CHOOSER_ACTION_SAVE,
                                        buttons = (gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_SAVE,gtk.RESPONSE_OK))
        dialog.set_default_response(gtk.RESPONSE_OK)
        response = dialog.run()
        if response == gtk.RESPONSE_OK:
            outputstream = ""
            for contact in  self._addressbook.get_all_contacts():
                outputstream = "%s%s,%s,%s,%s\n" % (outputstream,
                                                       contact.name,
                                                       contact.phone,
                                                       contact.email)
            fout = open ("%s.csv" % dialog.get_filename(), "w")
            fout.write (outputstream)
            fout.close()
        dialog.destroy()

    def __on_import_clicked (self, widget, event= None):
        dialog = gtk.FileChooserDialog (_(u"Seleccione el fichero destino para exportar"),
                                        self._main_window,
                                        gtk.FILE_CHOOSER_ACTION_OPEN,
                                        buttons = (gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_OPEN,gtk.RESPONSE_OK))
        dialog.set_default_response(gtk.RESPONSE_OK)
        response = dialog.run()
        if response == gtk.RESPONSE_OK:
            fin = open (dialog.get_filename(), "rb")
            for line in fin:
                import_contact = line.split(',')
                d = {}
                d["name"] = import_contact[0]
                d["phone"] = import_contact[1]
                d["email"] = import_contact[2].replace('\n', '')
                MDContact(**d)
                
            fin.close()
            self.__build_model() # TODO optimizar
            self.__check_button_sensitivity()
            
        dialog.destroy()

    def __on_copy_clicked (self, widget, event=None):
        selection = self._contact_treeview.get_selection()
        if selection.count_selected_rows() != 1:
            return
        
        model ,itera  = selection.get_selected()
        contact_id = model.get_value(itera,0)
        self._clip_board  = MDContact.get(contact_id)

    def __on_cut_clicked (self, widget, event=None):
        selection = self._contact_treeview.get_selection()
        if selection.count_selected_rows() != 1:
            return
        
        model ,itera  = selection.get_selected()
        contact_id = model.get_value(itera,0)
        target_contact  = MDContact.get(contact_id)
        self._clip_board  = target_contact
        target_contact.destroySelf()

        self.__build_model() # TODO optimizar
        self._contact_treeview.set_cursor(0)
        self.__check_button_sensitivity()

    def __on_paste_clicked (self, widget, event=None):
        if self._clip_board == None:
            return
        
        d = {}
        d["name"] = self._clip_board.name
        d["phone"] = self._clip_board.phone
        d["email"] = self._clip_board.email
        MDContact(**d)

        self.__build_model() # TODO optimizar
        self.__check_button_sensitivity()

    def __on_config_clicked (self, widget, event=None):
        self.preferences.show_all(tab=1)

    def __on_about_clicked (self, widget, event=None):
        self._about_dialog.run()
        self._about_dialog.hide()

    def __on_about_close_clicked (self, widget, event=None):
        self._about_dialog.hide()
        return True
        
    def show(self):
        self._main_window.show()
        self._main_window.deiconify()
        self.__check_button_sensitivity()
    
    def on_search_entry_changed(self,widget,event=None):
        #higliths
        #search_text = self._search_entry.get_text().strip()
        #if len(search_text) > 1 :
        #    # Pongo los renderers
        #    for col in self._contact_treeview.get_columns():
        #        renderers =  = col.get_cell()

        new_search_text = widget.get_text()
        old_search_text =  self._last_search_text
        self._last_search_text = new_search_text
        
        if len(new_search_text) > MIN_SEARCH_LEN or len(old_search_text) >= MIN_SEARCH_LEN :
            self._model.get_model().refilter()
            
        #self.__build_model() 

    def on_clear_search_entry_button_clicked(self, widget, event=None):
        self._search_entry.set_property("has-focus", True)
        self._search_entry.set_text("")
    
    def on_send_mail_button_clicked(self,widget,data=None):        
        contacts = self.__selected_contacts()
        if contacts is not None:
	    string = ""
	    for contact in contacts:
	        if len(contact.email) > 0:
		    if string == "":
		        string = contact.email
		    else:
		        string = string + ", " + contact.email

            gnome.url_show("mailto:%s" % string)
    
    def on_window_delete(self,widget,event=None):
        self._main_window.hide()
        return True

    
    def on_add_button_clicked(self,widget,event=None):
        response = self._contact_editor.run()
        
        if response != gtk.RESPONSE_OK:
            print _(u"Canceled")
            return

        # Creo uno nuevo
        d = self._contact_editor.get_dict()
        MDContact(**d)
        
        emsync.filter_contacts(self._addressbook.db_path, self.conf.get_country_code())
        
        self.__build_model() # TODO optimizar
        self.__check_button_sensitivity()
        
        
    def on_remove_button_clicked(self,widget,event=None):
        # aviso
    	contacts = self.__selected_contacts()
	length = len (contacts)
	if length == 0:
	    return
	elif length == 1:
	    message = _(u'¿Deseas eliminar el contacto "%s"?') % (contacts[0].name)
	else:
	    message = _(u'¿Deseas eliminar %d contactos?') % length

        dlg =gtk.MessageDialog(type=gtk.MESSAGE_WARNING,
                               buttons=gtk.BUTTONS_OK_CANCEL,
                               message_format=message)
        dlg.set_title(_(u"Eliminar contactois"))
        dlg.set_icon_from_file(MSD.icons_files_dir + "addressbook_16x16.png")
        response = dlg.run()
        dlg.destroy()

        if response != gtk.RESPONSE_OK:
            print _(u"Canceled")
            return

        for contact in contacts:
	    contact.destroySelf()
        self.__build_model() # TODO optimizar
        self.__check_button_sensitivity()
        self._contact_treeview.set_cursor(0)
        
    def on_edit_button_clicked(self,widget,event=None):
        selection = self._contact_treeview.get_selection()

        if selection.count_selected_rows()!= 1:
            return

        contacts = self.__selected_contacts()
        if len(contacts) == 1:
	    contact  =contacts[0]
            response = self._contact_editor.run(contact)
	    if response != gtk.RESPONSE_OK:
	        print "Canceled"
		return

	    d = self._contact_editor.get_dict()
	    contact.set(**d)
	    
	    emsync.filter_contacts(self._addressbook.db_path, self.conf.get_country_code())
	    
	    self.__build_model() # TODO optimizar
	    self.__check_button_sensitivity()

    def on_synchronize_button_clicked(self,widget,event=None):
        status = self.preferences.main_window_obj.mcontroller.dialer.status()
	if status != MobileManager.PPP_STATUS_CONNECTED:
	            mesg = gtk.MessageDialog(None,
		                             gtk.DIALOG_MODAL,
					     gtk.MESSAGE_ERROR,
					     gtk.BUTTONS_CLOSE)
		    mesg.set_icon_from_file(MSD.icons_files_dir + "addressbook_16x16.png")
		    mesg.set_markup(_(u"<b>No hay conexión establecida</b>"))
		    mesg.format_secondary_markup(_(u"Es necesario estar conectado para poder importar los contactos de Copiagenda movistar. Por favor, conecta previamente y vuelve a intentarlo."))
		    
		    if (mesg.run() != None):
		        mesg.destroy()
			
		    return


	username = self.conf.get_celular_info()[0]
	password = self.conf.get_celular_info()[1]			
	if username is None or password is None or username=="" or password=="" or self.conf.get_ask_password_activate():
		# dlg=gtk.MessageDialog(None, 0, gtk.MESSAGE_ERROR, gtk.BUTTONS_OK, _(u"Faltan datos de usuario y contraseña para utilizar el servicio Copiagenda.\nPor favor, introduce estos datos en las opciones de configuración y vuelve a intentarlo."))
		# dlg.set_title("Importar")
		# dlg.set_icon_from_file(MSD.icons_files_dir + "synchronize_32x32.png")
		# dlg.run()
		# dlg.destroy()
		
		# self.preferences.show_all(3)
		MSD.MSDSecurityManager(self.conf).run_security_dialog()
				
	username = self.conf.get_celular_info()[0]
	password = self.conf.get_celular_info()[1]			

	if username is not None and password is not None and username!="" and password!="":
		sync=emsync.Sync()
		if sync != None:
			sync.start(
				self._addressbook.db_path,
				self.conf.get_syncml_server(), # . "195.235.160.141",
				self.conf.get_syncml_service(), # "syncml",
				self.conf.get_syncml_resource(), # "./pab",
				"1234567890",
				username, # "609000055",
				password, # "1234567890123456789",
				username) # "609000055")
		
			progress = MDImportProgress(sync)
			# progress.run()
				# state = emsync.Sync.State_Connecting
			# while state != sync.State_Finished and state != emsync.Sync.State_FinishedWithError and state != emsync.Sync.State_Cancelled and state != emsync.Sync.State_FinishedWithPartialDownload :
			# 	state = sync.get_state()

			emsync.filter_contacts(self._addressbook.db_path, self.conf.get_country_code())
				
			self.__build_model()
		else:
			dlg=gtk.MessageDialog(None, 0, gtk.MESSAGE_ERROR, gtk.BUTTONS_OK, _(u"Ha ocurrido un problema al intentar importar.\nReinstalar la aplicación podría resolver el problema."))
			dlg.set_title("Importar")
			dlg.set_icon_from_file(MSD.icons_files_dir + "synchronize_32x32.png")
			dlg.run()
			dlg.destroy()				

    def __on_show_help_clicked (self, widget, event=None):
        dir_name = os.path.dirname(MSD.help_uri)
        help_file = os.path.join(dir_name, "em_61.htm")
        ret = os.popen("gconftool-2 -g /desktop/gnome/applications/browser/exec")
        url_cmd = ret.readline().split()
        if len(url_cmd) > 0 :
            os.system("%s 'file://%s' &" % (url_cmd[0], help_file))
            
def get_addressbook_controller(conf, preferences, address_book=None):
    if address_book is not  None:   
        return MDAddressBookController(address_book, conf, preferences)

    user_book = get_user_addressbook()
    if user_book is None:
        return None
    
    return MDAddressBookController(user_book, conf, preferences)

