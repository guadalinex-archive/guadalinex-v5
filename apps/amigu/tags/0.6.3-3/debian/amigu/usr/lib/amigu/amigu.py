#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pygtk
pygtk.require('2.0')
import sys, os, re, gobject, gtk, time, threading, stat, traceback, syslog, shutil
import gettext
from gnome import url_show
import mail, webbrowser, messenger, settings, computer
from folder import *

_ = gettext.gettext
gettext.textdomain("amigu")
gettext.bindtextdomain("amigu", "./translations")


dir_imagenes = "/usr/share/pixmaps/amigu"
#Initializing the gtk's thread engine
gtk.gdk.threads_init()
barra = None
mensaje_progreso = None
version = "AMIGU 0.6.3"
ver = "0.6.3"



class barra_progreso(threading.Thread):
	"""Clase encargada de actualizar la información de la barra de progreso"""
	
	#Thread event, stops the thread if it is set.
	stopthread = threading.Event()
	
	#Importing the progressbar widget from the global scope
	global barra
	global detalle
	global text_view
	
	def __init__(self, n, pipe_r):
		"""Constructor que recibe el número de acciones a realizar y la tuberia de lectura para la etapa actual"""
		threading.Thread.__init__(self)
		self.n_tasks = float(n)
		self.pipe_r = pipe_r
		
	
	def run(self):
		"""Actualiza la barra de progreso mientras el hilo siga vivo"""
		
		#While the stopthread event isn't setted, the thread keeps going on
		while not self.stopthread.isSet() :
			
			msg = os.read(self.pipe_r, 100)
			
			# Acquiring the gtk global mutex
			gtk.gdk.threads_enter()
			#Setting a random value for the fraction
			por = barra.get_fraction() + (1/(self.n_tasks + 0.5))
			if por > 1:
				por = 0.0
			barra.set_fraction(por)
			if msg:
				barra.set_text(msg)
			# Releasing the gtk global mutex
			gtk.gdk.threads_leave()
			if msg == "Finalizado":
				time.sleep(0.1)
				msg = os.read(self.pipe_r, 2000)
				detalle.set_text(msg)
				self.stop()
			#Delaying 100ms until the next iteration
			time.sleep(0.1)
			
	def stop(self):
		"""Detiene la actualización de la barra de progreso"""
		self.stopthread.set()
		gtk.gdk.threads_enter()
		barra.set_fraction(1.0)
		gtk.gdk.threads_leave()

class Asistente:
	"""GUI for AMIGU
	Implementación del asistente de migración de Guadalinex sobre pygtk"""
	
	def excepthook(self, exctype, excvalue, exctb):
		"""Manejador de excepciones"""
		if (issubclass(exctype, KeyboardInterrupt) or issubclass(exctype, SystemExit)):
			return
		tbtext = ''.join(traceback.format_exception(exctype, excvalue, exctb))
		syslog.syslog(syslog.LOG_ERR, "Exception in GTK frontend (invoking crash handler):")
		for line in tbtext.split('\n'):
			syslog.syslog(syslog.LOG_ERR, line)
		print >>sys.stderr, ("Exception in GTK frontend (invoking crash handler):")
		print >>sys.stderr, tbtext
	
		#self.dialogo_error(tbtext)
		if os.path.exists('/usr/share/apport/apport-gtk'):
			self.previous_excepthook(exctype, excvalue, exctb)
		tbtext = _('Se ha producido un error durante la ejecucción del programa que impide continuar.') + _('Si dicho fallo se repite por favor comuníquelo al equipo de desarrolladores adjuntando el siguiente error:') + '\n\n' + tbtext
		error_dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, message_format=tbtext, buttons=gtk.BUTTONS_CLOSE)
		error_dlg.run()
		error_dlg.destroy()
		sys.exit(1)
	
	def __init__(self):
		"""Constructor de la interfaz"""
		self.previous_excepthook = sys.excepthook
        	sys.excepthook = self.excepthook
		
		# Main Window
		self.window = gtk.Window()
		self.window.set_title(_("Asistente de Migración a Guadalinex"))
		self.window.set_default_size(700, 570)
		self.window.set_border_width(0)
		self.window.move(150, 50)
		self.window.connect("destroy", self.destroy)
		self.window.set_icon_from_file(os.path.join(dir_imagenes, "icon_paginacion.png"))
		#self.window.set_icon_from_file('/home/fernando/Desktop/paquetes/amigu-0.5.3/imagenes/icon_paginacion.gif')
		# Atributos
		self.paso = 1
		self.usuarios = {}
		self.selected_user = None
		self.resumen = []
		self.tasks = []
		self.detalles = ""
		self.url = "http://forja.guadalinex.org/webs/amigu/"
		self.watch = gtk.gdk.Cursor(gtk.gdk.WATCH)
		gtk.about_dialog_set_url_hook(self.open_url, self.url)
		try:
			gtk.link_button_set_uri_hook(self.open_url, self.url)
		except:
			print _("Está usando una versión anterior a python-gtk2 2.10")
		# ventana principal
		contenedor = gtk.VBox(False, 1)#Creo la caja vertical principal
		self.inicio = gtk.VBox(False, 1)
		self.segunda = gtk.VBox(False, 1)
		self.tercera = gtk.HBox(False, 1)
		self.cuarta = gtk.VBox(False, 1)
		self.quinta = gtk.VBox(False, 1)
		separador1 = gtk.HSeparator()
		separador2 = gtk.HSeparator()
		self.tooltips = gtk.Tooltips()
		principal = gtk.HBox(False, 1)#Creo lo que se va a quedar siempre en la ventana
		principal0 = gtk.HBox(False, 1)#Creo lo que se va a quedar siempre en la ventana
		principal1 = gtk.VBox(False, 1)#Creo lo que va a ir variando
		principal2 = gtk.HBox(False, 1)#Creo lo que se va a quedar siempre en la ventana
		principal2ver1 = gtk.HBox(False, 1)
		principal2ver2 = gtk.HBox(True, 1)
		principal2ver2.set_size_request(550,60)
		principal2ver2.set_border_width(10)
		image = gtk.Image()
		image.set_from_file(os.path.join(dir_imagenes, "cab_amigu.png"))
		#image.set_from_file('/home/fernando/Desktop/paquetes/amigu-0.5.3/imagenes/cab_amigu.jpg')
		image1 = gtk.Image()
		image1.set_from_file(os.path.join(dir_imagenes, "icon_paginacion.png"))
		#image1.set_from_file('/home/fernando/Desktop/paquetes/amigu-0.5.3/imagenes/icon_paginacion.gif')

		# botones
		self.boton = gtk.Button(stock = gtk.STOCK_CANCEL)
		self.boton1 = gtk.Button(stock = gtk.STOCK_GO_BACK)
		self.boton2 = gtk.Button(stock = gtk.STOCK_GO_FORWARD)
		self.boton3 = gtk.Button(stock = gtk.STOCK_APPLY)
		self.boton4 = gtk.Button(stock = gtk.STOCK_QUIT)
		self.boton5 = gtk.Button(label=_("Acerca de"))
		
		self.etapa = gtk.Label()
		self.etapa.set_use_markup(True)
		self.etapa.set_markup("<span face = \"arial\" size = \"8000\" foreground = \"chocolate\"><b>Paso %d de 5</b></span>"%self.paso)

		# añadir
		self.labelpri = gtk.Label("")
		espacio = gtk.Label()
		self.labelpri.set_line_wrap(True)
		self.labelpri.set_justify(gtk.JUSTIFY_LEFT)
		self.labelpri.set_use_markup(True)
		principal.pack_start(image, False, False, 0)
		principal0.pack_start(self.labelpri, False, False, 10)
		principal1.pack_start(self.inicio, True, False, 0)
		principal2.pack_start(principal2ver1, False, False, 0)
		principal2.pack_start(principal2ver2, True, False, 0)
		principal2ver1.pack_start(image1, True, True, 0)
		principal2ver1.pack_start(self.etapa, False, False, 1)
		principal2ver2.pack_start(self.boton1, False, False, 0)
		principal2ver2.pack_start(self.boton2, False, False, 0)
		principal2ver2.pack_start(self.boton3, False, False, 0)
		principal2ver2.pack_start(self.boton4, False, False, 0)
		principal2ver2.pack_start(self.boton5, False, False, 0)
		principal2ver2.pack_start(espacio, True, True, 2)
		principal2ver2.pack_start(self.boton, True, False, 5)
		contenedor.pack_start(principal, False, False, 1)
		contenedor.pack_start(separador1, False, True, 10)
		contenedor.pack_start(principal0, False, False, 1)
		contenedor.pack_start(principal1, True, True, 1)
		contenedor.pack_start(separador2, False, True, 10)
		contenedor.pack_start(principal2, False, True, 1)
		principal1.pack_start(self.segunda, True, False, 1)
		principal1.pack_start(self.tercera, True, False, 1)
		principal1.pack_start(self.cuarta, True, False, 1)
		principal1.pack_start(self.quinta, True, False, 1)
		self.window.add(contenedor)#Añado a la ventana el contenedor
		
		# eventos
		self.boton1.connect("clicked", self.etapa_anterior)
		self.boton2.connect("clicked", self.etapa_siguiente)
		self.boton3.connect("clicked", self.dialogo_confirmacion)
		self.boton4.connect("clicked", self.destroy)
		self.boton.connect_object("clicked", self.dialogo_cancelacion, self.window)
		self.boton5.connect_object("clicked", self.about, self.window)
		
		# mostrar ventana
		self.window.show_all()
		self.boton3.hide()
		self.boton4.hide()


################Primera Ventana	

		# crear
		label1 = gtk.Label()
		label1.set_use_markup(True)
		label1.set_line_wrap(True)
		label1.set_justify(gtk.JUSTIFY_CENTER)
		label1.set_markup('<b>'+_('Bienvenido al Asistente de MIgración de GUadalinex - AMIGU') + '\n\n' + _('Este asistente le guiará durante el proceso de migración de sus documentos y configuraciones de su sistema operativo Windows.') + '\n' + _('Pulse el botón Adelante para comenzar.') + '</b>')

		# añadir
		self.inicio.pack_start(label1, True, True, 10)
		
		# mostrar
		self.boton1.hide_all()
		self.inicio.show_all()


################Segunda Ventana

		#crear
		self.segundahor1 = gtk.HBox(False, 10)
		self.segundahor2 = gtk.HBox(False, 10)
		label2 = gtk.Label()
		label2.set_use_markup(True)
		label2.set_markup('<b>'+_('Seleccione uno de los usuarios de Windows')+'</b>')
		frame1 = gtk.Frame(_("Información"))
		sw = gtk.ScrolledWindow()
		sw.set_size_request(600, 175)
		sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		textview = gtk.TextView()
		textview.set_editable(False)
		self.textbuffer = textview.get_buffer()
		self.combo = gtk.combo_box_new_text()
		self.combo.append_text(_('Usuarios de Windows:'))
		self.combo.set_active(0)
		self.combo.connect("changed", self.actualizar_info)
		

		# añadir
		sw.add(textview)
		frame1.add(sw)
		self.segundahor1.pack_start(label2, False, True, 10)
		self.segundahor1.pack_start(self.combo, False, False, 10)
		self.segundahor2.pack_start(frame1, True, True, 10)
		self.segunda.pack_start(self.segundahor2, True, False, 10)
		self.segunda.pack_start(self.segundahor1, True, True, 10)
		
	


###############################Tercera Ventana

		# crear
		self.terceraver1 = gtk.HBox(False, 10)
		self.terceraver2 = gtk.HBox(False, 10)
		self.tercerahor1 = gtk.VBox(False, 10)
		self.tercerahor2 = gtk.VBox(False, 10)
		frame2 = gtk.Frame(_("Destino"))
		self.entry = gtk.Entry()
		self.entry.set_max_length(50)
		f = folder(os.path.expanduser('~'))
		self.required_space = 0
		self.espaciolib = gtk.Label(_("Espacio libre en disco") + ": %dKB" % f.get_free_space())
		self.espacioreq = gtk.Label(_("Espacio requerido") + ": %dKB" % self.required_space)
		self.entry.connect("activate", self.actualizar_espacio_libre, self.entry)
		self.entry.set_text(os.path.expanduser('~'))
		boton22 = gtk.Button(label = _("Seleccionar otra carpeta"), stock = None)
		frame5 = gtk.Frame(_("Opciones de migración"))
		self.arbol_inicializado = False
		self.tree_options = gtk.TreeStore( gobject.TYPE_STRING, gobject.TYPE_BOOLEAN, gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_STRING)
		self.sw3 = gtk.ScrolledWindow()
		self.sw3.set_size_request(280, 200)
		self.sw3.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		self.sw3.add(self.generar_vista(self.tree_options))
		
		# añadir
		self.tercera.pack_start(self.terceraver1, False, False, 0)
		self.tercera.pack_start(self.terceraver2, True, True, 0)
		self.terceraver1.pack_start(self.tercerahor1, False, False, 10)
		self.terceraver2.pack_start(self.tercerahor2, True, True, 10)
		frame5.add(self.sw3)
		frame2.add(self.entry)
		self.tercerahor1.pack_start(frame2, True, False, 0)
		self.tercerahor1.pack_start(boton22, True, False, 0)
		self.tooltips.set_tip(boton22, _("Elija la carpeta Destino donde quiere guardar los archivos"))
		boton22.connect_object("clicked", self.buscar, self.window)
		self.tercerahor1.pack_start(self.espaciolib, True, True, 5)
		self.tercerahor1.pack_start(self.espacioreq, True, True, 5)
		self.tercerahor2.pack_start(frame5, True, False, 0)


################Cuarta Ventana

		# crear
		self.cuartahor1 = gtk.HBox(False, 10)
		frame3 = gtk.Frame(_("Tareas a realizar"))
		sw1 = gtk.ScrolledWindow()
		sw1.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		sw1.set_size_request(600, 225)
		textview1 = gtk.TextView()
		self.textbuffer1 = textview1.get_buffer()
		textview1.set_editable(False)

		#añadir
		self.cuarta.pack_start(self.cuartahor1, True, True, 0)
		self.cuartahor1.pack_start(frame3, True, True, 10)
		frame3.add(sw1)
		sw1.add(textview1)


################Quinta Ventana
		
		# crear
		self.quintahor1 = gtk.VBox(False, 10)
		frame4 = gtk.Frame(_("Detalles de tareas"))
		sw4 = gtk.ScrolledWindow()
		sw4.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		sw4.set_size_request(580, 200)
		textview4 = gtk.TextView()
		self.textbuffer4 = textview4.get_buffer()
		self.textbuffer4.set_text(_('Comenzando el proceso de migración...') + '\n ' + _('Esto puede llevar cierto tiempo en función del tamaño de los archivos y de la velocidad de su equipo.') + '\n ' + _('Por favor espere...'))
		textview4.set_editable(False)
		self.progreso = gtk.ProgressBar(None)
		self.progreso.set_fraction(0.0)
		self.progreso.set_orientation(gtk.PROGRESS_LEFT_TO_RIGHT)
		self.progreso.set_text(_("Realizando las tareas solicitadas..."))
		
		# añadir
		self.quinta.pack_start(self.quintahor1, True, False, 10)
		self.quintahor1.pack_start(frame4, True, True, 10)
		self.quintahor1.pack_start(self.progreso, False, False, 5)
		frame4.add(sw4)
		sw4.add(textview4)
		global barra
		barra = self.progreso
		global text_view
		text_view = textview4	
		global detalle
		detalle = self.textbuffer4
		try:
			child = gtk.EventBox()
			b =  gtk.LinkButton(self.url, label=_("Más información sobre Amigu"))
			child.add(b)
			text_view.add_child_in_window(child, gtk.TEXT_WINDOW_TEXT, 480, 173)
		except: 
			pass
		

########Cuadros de dialogo

	def about(self, widget):
		dialog = gtk.AboutDialog()
		dialog.set_name("AMIGU")
		dialog.set_version(ver)
		dialog.set_copyright("Copyright © 2006-2007 Fernando Ruiz. Emilia Abad")
		dialog.set_website(self.url)
		dialog.set_website_label(self.url)
		dialog.set_authors([
			_("Programadores") + ':',
			'Emilia Abad Sánchez <eabad@cassfa.com>\n',
			'Fernando Ruiz Humanes <fruiz@cassfa.com>\n'
			#_('Contributors:'), # FIXME: remove ":"
		])
		dialog.set_artists([_("Diseñadora gráfica") + ':',
			'Emilia Abad Sánchez <eabad@cassfa.com>\n',
			_("Logo e icono") + " por Sadesi"
		])
		dialog.set_translator_credits(_("Este programa aún no ha sido traducido a otros idiomas"))
		logo_file = os.path.abspath(os.path.join(dir_imagenes, 'icon_paginacion.png'))
		logo = gtk.gdk.pixbuf_new_from_file(logo_file)
		dialog.set_logo(logo)
		if os.path.isfile('/usr/share/common-licenses/GPL'):
			dialog.set_license(open('/usr/share/common-licenses/GPL').read())
		else:
			dialog.set_license("This program is released under the GNU General Public License.\nPlease visit http://www.gnu.org/copyleft/gpl.html for details.")
		dialog.set_comments(_("asistente a la migración de Guadalinex"))
		dialog.run()
		dialog.destroy()

	def dialogo_creditos(self,widget):
		# crear
		global version
		self.creditos = gtk.Dialog(_("Acerca de AMIGU"), self.window, 100, None)
		self.logo = gtk.Image()
		self.creditos.set_icon_from_file(os.path.join(dir_imagenes, "icon_paginacion.png"))
		self.informacion = gtk.Label("")
		self.informacion.set_use_markup(True)
		self.informacion.set_markup('<span face="arial" size="16000" foreground="chocolate"><b>%s</b></span>'%version)

		self.informacion1 = gtk.Label("")
		self.informacion1.set_use_markup(True)
		self.informacion1.set_markup('<span><b>_(Asistente de MIgracion a GUadalinex)</b></span>')
		self.informacion2 = gtk.Label("")
		self.informacion2.set_use_markup(True)
		self.informacion2.set_markup('<span>Emilia Abad Sánchez <i>(eabad@cassfa.com)</i>\nFernando Ruiz Humanes<i>(fruiz@cassfa.com)</i></span>')

		self.informacion3 = gtk.Label("")
		self.informacion3.set_use_markup(True)
		self.informacion3.set_markup('<span face="arial" size="14000" foreground="blue">'+self.url+'</span>')
		

		self.cerrar = gtk.Button(stock = gtk.STOCK_CLOSE)
		self.creditosv = gtk.VBox(False, 10)
		self.creditosh1 = gtk.HBox(False, 10)
		self.creditosh2 = gtk.HBox(False, 10)
		self.creditosh3 = gtk.HBox(False, 10)
		self.creditosh4 = gtk.HBox(False, 10)
		self.creditosh5 = gtk.HBox(False, 10)
		self.creditosh6 = gtk.HBox(False, 10)
		self.logo.set_from_file(os.path.join(dir_imagenes, "icon_paginacion.png"))
		
		# añadir
		self.creditosv.pack_start(self.creditosh1, True, False, 10)
		self.creditosv.pack_start(self.creditosh2, True, False, 10)
		self.creditosv.pack_start(self.creditosh3, True, False, 10)
		self.creditosv.pack_start(self.creditosh4, True, False, 10)
		self.creditosv.pack_start(self.creditosh5, True, False, 10)
		self.creditosv.pack_start(self.creditosh6, True, False, 10)
		self.creditosh1.pack_start(self.logo, True, False, 10)
		self.creditosh2.pack_start(self.informacion, True, False, 10)
		self.creditosh3.pack_start(self.informacion1, True, False, 10)
		self.creditosh4.pack_start(self.informacion2, True, False, 10)
		self.creditosh5.pack_start(self.informacion3, True, False, 10)
		self.creditosh6.pack_start(self.cerrar, True, False, 10)
		self.creditos.action_area.pack_start(self.creditosv, True, False, 10)
		
		# eventos
		self.cerrar.connect_object("clicked", gtk.Widget.destroy, self.creditos)
		
		#mostrar
		self.creditos.show_all()
		
	def dialogo_advertencia(self, mensaje = ""):
		"""Muestra mensajes de aviso"""
		# crear
		self.advertencia = gtk.Dialog(_("Aviso"), self.window, 100, None)
		self.logoad = gtk.Image()
		iconad = self.advertencia.render_icon(gtk.STOCK_DIALOG_WARNING, 1)
		mensaje = gtk.Label(mensaje)
		self.aceptar = gtk.Button(stock = gtk.STOCK_OK)
		self.advertenciav = gtk.VBox(False, 10)
		self.advertenciah1 = gtk.HBox(False, 10)
		self.advertenciah2 = gtk.HBox(False, 10)
		
		# añadir
		self.advertencia.set_icon(iconad)
		self.advertencia.action_area.pack_start(self.advertenciav, True, False, 10)
		self.advertenciav.pack_start(self.advertenciah1, True, False, 10)
		self.advertenciav.pack_start(self.advertenciah2, True, False, 10)
		self.advertenciah1.pack_start(self.logoad, True, False, 10)
		self.advertenciah1.pack_start(mensaje, True, False, 10)
		self.advertenciah2.pack_start(self.aceptar, True, False, 10)
		self.logoad.set_from_stock(gtk.STOCK_DIALOG_WARNING, 6)
		
		# eventos
		self.aceptar.connect_object("clicked", gtk.Widget.destroy, self.advertencia)
		
		# mostrar
		self.advertencia.show_all()
		
	def dialogo_cancelacion(self, widget):
		"""Muestra el cuadro diálogo de cancelación del proceso."""
		# crear
		self.cancelar = gtk.Dialog(_("Alerta"), self.window, 100, None)
		self.logo = gtk.Image()
		icon = self.cancelar.render_icon(gtk.STOCK_DIALOG_QUESTION, 1)
		self.cancelar.set_icon(icon)
		self.pregunta = gtk.Label(_("¿Está seguro que desea salir del asistente de migración?"))
		self.si = gtk.Button(stock = gtk.STOCK_OK)
		self.no = gtk.Button(stock = gtk.STOCK_CANCEL)
		self.cancelarv = gtk.VBox(False, 10)
		self.cancelarh1 = gtk.HBox(False, 10)
		self.cancelarh2 = gtk.HBox(False, 10)
		self.logo.set_from_stock(gtk.STOCK_DIALOG_QUESTION, 6)
		
		# añadir
		self.cancelarv.pack_start(self.cancelarh1, True, False, 10)
		self.cancelarv.pack_start(self.cancelarh2, True, False, 10)
		self.cancelarh1.pack_start(self.logo, True, False, 10)
		self.cancelarh1.pack_start(self.pregunta, True, False, 10)
		self.cancelarh2.pack_start(self.si, True, False, 10)
		self.cancelarh2.pack_start(self.no, True, False, 10)
		self.cancelar.action_area.pack_start(self.cancelarv, True, False, 10)
		
		# eventos
		self.si.connect_object("clicked", gtk.Widget.destroy, self.window)
		self.no.connect_object("clicked", gtk.Widget.destroy, self.cancelar)
				
		#mostrar
		self.cancelar.show_all()
	
	def dialogo_confirmacion(self, widget):
		"""Muestra el diálogo de confirmación antes de empezar la migración"""
		# crear
		self.confirmar = gtk.Dialog(_("Confirmación"), self.window, 100, None)
		logo = gtk.Image()
		icon = self.confirmar.render_icon(gtk.STOCK_DIALOG_QUESTION, 1)
		self.confirmar.set_icon(icon)
		pregunta = gtk.Label(_('El asistente ha reunido la información necesaria para realizar la migración.') + '\n' + _('Es aconsejable cerrar todas las aplicaciones antes continuar.') + '\n' + _('¿Desea comenzar la copia de archivos/configuraciones?'))
		si = gtk.Button(stock = gtk.STOCK_OK)
		no = gtk.Button(stock = gtk.STOCK_CANCEL)
		self.confirmarv = gtk.VBox(False, 10)
		self.confirmarh1 = gtk.HBox(False, 10)
		self.confirmarh2 = gtk.HBox(False, 10)
		logo.set_from_stock(gtk.STOCK_DIALOG_QUESTION, 6)
		
		# añadir
		self.confirmarv.pack_start(self.confirmarh1, True, False, 10)
		self.confirmarv.pack_start(self.confirmarh2, True, False, 10)
		self.confirmarh1.pack_start(logo, True, False, 10)
		self.confirmarh1.pack_start(pregunta, True, False, 10)
		self.confirmarh2.pack_start(si, True, False, 10)
		self.confirmarh2.pack_start(no, True, False, 10)
		self.confirmar.action_area.pack_start(self.confirmarv, True, False, 10)
		
		# eventos
		si.connect_object("clicked", self.etapa_siguiente, self.confirmar)
		si.connect_object("clicked", gtk.Widget.destroy, self.confirmar)
		no.connect_object("clicked", gtk.Widget.destroy, self.confirmar)
		
		# mostrar
		self.confirmar.show_all()
		
	
	
	def buscar(self, widget):
		"""Muestra el cuadro diálogo para seleccionar la ubicación de destino de los archivos"""
		# crear
		dialog = gtk.FileChooserDialog(_("Destino"), None, gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER, (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK)) 
		dialog.set_default_response(gtk.RESPONSE_OK)
		dialog.set_current_folder(os.path.expanduser('~'))
		
		# run
		response = dialog.run()
		if response == gtk.RESPONSE_OK:
			self.entry.set_text(dialog.get_current_folder())
			self.actualizar_espacio_libre(self.window, self.entry)
		dialog.destroy()
	
########Eventos

	def open_url(self,dialog, url, widget):
		url_show(url)
		
	def destroy(self, widget, data=None):
		"""Finaliza la ejecución del asistente y limpiar los archivos temporales"""
		print _("Saliendo del asistente")
		if self.usuarios:
			print _("Eliminando archivos temporales...")
			for name, u in self.usuarios.iteritems():
				u.clean()
		gtk.main_quit()
		
	def etapa_siguiente(self, widget):
		"""Muestra la siguiente etapa"""
		self.boton.window.set_cursor(self.watch)
		self.paso += 1
		gobject.idle_add(self.mostrar_paso)
		
		
	def etapa_anterior(self, widget):
		"""Muestra la etapa previa"""
		self.boton.window.set_cursor(self.watch)
		self.paso -= 1
		gobject.idle_add(self.mostrar_paso)
		
		
	def actualizar_info(self, entry):
		"""Actualiza la información asociada al usuario seleccionado"""
		self.boton2.set_sensitive(False)
		model = entry.get_model()
		index = entry.get_active()
		seleccion = model[index][0]
		if seleccion and (not self.selected_user or self.selected_user.name != seleccion):
			self.selected_user = self.usuarios[seleccion]
			self.arbol_inicializado = False
			self.textbuffer.set_text(self.selected_user.get_info())
			self.boton2.set_sensitive(True)
		
		
	def actualizar_espacio_libre(self, widget, entry):
		"""Actualiza el espacio libre en disco"""
		destino = entry.get_text()
		if os.path.exists(destino):
			d = folder(destino)
			self.espaciolib.set_text(_("Espacio libre en disco: %dKB") % d.get_free_space())


	def marcar_opcion( self, cell, path, model):
		"""Establece el estado marcado/desmarcado a las opciones de migración y sus descendientes"""
		iterator = model.get_iter(path)
		checked = not cell.get_active()
		self.view.expand_row(path, True)
		self.recorrer_arbol(iterator, model, checked)
		self.espacioreq.set_text(_("Espacio requerido: %dKB") % self.required_space)
		return
	
	
########Metodos		
	def mostrar_paso(self):
		"""Actualiza el contenido del asistente en función del paso actual"""
		# Ocultar todo
		self.inicio.hide()
		self.segunda.hide()
		self.tercera.hide()
		self.cuarta.hide()
		self.quinta.hide()
		self.boton1.show_all()
		self.boton1.set_sensitive(True)
		self.boton2.show()
		self.boton2.set_sensitive(False)
		self.boton3.hide()
		self.boton5.hide()
		self.etapa.set_markup("<span face=\"arial\" size=\"8000\" foreground=\"chocolate\"><b>"+_('Paso %d de 5')%self.paso+"</b></span>")
		
		if self.paso == 1:
			self.inicio.show()
			self.labelpri.set_markup('')
			self.boton1.hide_all()
			self.boton2.set_sensitive(True)

		elif self.paso == 2:
			self.segunda.show_all()
			self.labelpri.set_markup('<span face="arial" size="12000" foreground="chocolate"><b>'+_('SELECCIÓN DE USUARIO')+'</b></span>')
			if self.selected_user:
				self.boton2.set_sensitive(True)
			if not self.usuarios:
				self.textbuffer.set_text(_('Buscando usuarios de Windows. Por favor espere...'))
				self.boton.window.set_cursor(self.watch)
				gobject.idle_add(self.buscar_usuarios)

		elif self.paso == 3:
			self.labelpri.set_markup('<span face="arial" size="12000" foreground="chocolate"><b>'+_('OPCIONES DE MIGRACIÓN')+'</b></span>')
			if not self.arbol_inicializado:
				self.generar_arbol()
				self.arbol_inicializado = True
			self.boton2.set_sensitive(True)
			self.tercera.show_all()

		elif self.paso == 4:
			self.destino = self.entry.get_text()
			self.boton2.set_sensitive(True)
			if self.destino:
				f = folder(self.destino)
				if not f or not f.path:
					self.paso = 3
					self.dialogo_advertencia(_("Ruta de destino no válida"))
					self.tercera.show()
				elif not f.is_writable():
					self.paso = 3
					self.dialogo_advertencia(_("No dispone de permiso de escritura para la ubicación seleccionada"))
					self.tercera.show()
				elif f.get_free_space < self.required_space:
					self.paso = 3
					self.dialogo_advertencia(_("Espacio en disco insuficiente.") + '\n' + _('Seleccione otra ubicación de destino o cambie sus opciones de migración'))
					self.tercera.show()
				else:
					self.labelpri.set_markup('<span face="arial" size="12000" foreground="chocolate"><b>'+_('RESUMEN DE TAREAS')+'</b></span>')
					self.generar_resumen(self.tree_options)
					self.boton2.hide()
					self.boton3.show_all()
					self.cuarta.show_all()
			else:
				self.paso = 3
				self.dialogo_advertencia(_("Introduzca una ubicación de destino"))
				self.tercera.show()

		elif self.paso == 5:
			self.labelpri.set_markup('<span face="arial" size="12000" foreground="chocolate"><b>'+_('DETALLES DE LA MIGRACIÓN')+'</b></span>')
			self.quinta.show_all()
			self.boton1.hide_all()
			self.boton2.hide_all()
			self.boton5.show()
			time.sleep(0.2)
			gobject.idle_add(self.aplicar)
		self.boton.window.set_cursor(None)
	
	def generar_arbol(self):
		"""Genera el árbol de opciones para el usario seleccionado"""
		print _("Generando opciones de migración...")
		# limpiar variables
		self.tree_options.clear() 
		self.tasks = []
		self.resumen = []
		# Generar vista
		self.view = self.generar_vista(self.tree_options)
		name_usr = self.tree_options.append(None, [self.selected_user.get_name(), None, None, _('Usuario seleccionado'), None])
		
		# Files
		parent = self.tree_options.append(name_usr , [_("Archivos"), None, None, _('Migrar archivos'), None] )
		self.tree_options.append( parent, [_("Mis documentos"), None, str(self.selected_user.folders['Personal'].get_size()), _('Archivos personales: ')+ self.selected_user.folders['Personal'].get_info(), 'documentos'] )
		self.tree_options.append( parent, [_("Escritorio"), None, str(self.selected_user.folders['Desktop'].get_size()), _('Archivos del escritorio: ')+self.selected_user.folders['Desktop'].get_info(), 'escritorio'] )
		
		# eMule
		if 'eMule' in self.selected_user.folders.keys():
			emule_inc = self.selected_user.get_incoming_path()
			emule_temp = self.selected_user.get_temp_path()
			self.tree_options.append( parent, [_("Descargados eMule"), None, str(emule_inc.get_size()), _('Archivos descargados: ')+ emule_inc.get_info(), 'descargados'] )
			self.tree_options.append( parent, [_("Temporales eMule"), None, str(emule_temp.get_size()), _('Archivos temporales: ')+ emule_temp.get_info(), 'temporales'] )
		parent = self.tree_options.append(name_usr , [_("Configuraciones"), None, None, _('Migrar configuraciones'), None] )
		
		# Thundebird
		t = mail.thunderbird()
		if t.is_installed_on_Windows(self.selected_user.folders['AppData']):
			mthun = self.tree_options.append(parent, [_("Mozilla-Thunderbird"), None, None, _('Cuentas de correo y libreta de direcciones (sobreescribirá la configuración existente)'), 'thunderbird'] )
			
		# Outlook
		#print _('Buscando cuentas de correo...')
		n_ou, n_oue = self.selected_user.num_OUTLOOK_accounts()
		self.buzones_ou, self.buzones_oue, oue = None, None, None
		pstpath = folder(os.path.join(self.selected_user.folders['Local AppData'].path, 'Microsoft', 'Outlook'), False)
		dbxpath = folder(os.path.join(self.selected_user.folders['Local AppData'].path, 'Identities'), False)
		if pstpath.path: 
			self.buzones_ou = pstpath.search_by_ext('.pst') + self.selected_user.folders['Personal'].search_by_ext('.pst')
		if dbxpath.path:
			self.buzones_oue = dbxpath.search_by_ext('.dbx')
		if n_oue or self.buzones_oue: oue = self.tree_options.append(parent, [_("OutlookExpress"), None, None, _('Cuentas de correo'), None] )
		if n_ou or self.buzones_ou: ou = self.tree_options.append(parent, [_("Outlook"), None, None, _('Cuentas de correo'), None] )
		# Mailboxes
		if self.buzones_ou:
			tam = 0
			for b in self.buzones_ou:
				try:
					tam = tam + os.stat(b)[stat.ST_SIZE]
				except: 
					pass
			self.tree_options.append(ou, [_('Carpetas de correo'), None, tam/1024, _('Carpetas de correo de Outlook'), 'oumails'])
		if self.buzones_oue:
			tam = 0
			for b in self.buzones_oue:
				try:
					tam = tam + os.stat(b)[stat.ST_SIZE]
				except: 
					pass
			self.tree_options.append(oue, [_('Carpetas de correo'), None, tam/1024, _('Carpetas de correo de Outlook Express'), 'ouemails'])
		#Addressbook
		self.adbook = os.path.join(self.selected_user.folders['AppData'].path, 'Microsoft', 'Address Book', self.selected_user.get_name()+'.wab')
		#print self.adbook
		tam = 0
		if os.path.exists(self.adbook) and oue:
			try:
				tam = tam + os.stat(self.adbook)[stat.ST_SIZE]
			except: 
				pass
			self.tree_options.append(oue, [_('Libreta de direcciones'), None, tam/1024, _('Libreta de direcciones de Outlook Express'), 'oueaddressbook'])
		#Accounts
		seguir_buscando, i = True, 0
		while seguir_buscando:
			i = i + 1
			if n_oue:
				account = self.selected_user.get_OUTLOOKexpress_account(i)
				if account: n_oue = n_oue - 1
				if account and 'Account Name' in account.keys():
					self.tree_options.append(oue, [account['Account Name'], None, None, _('Cuenta de correo <' + account['SMTP Email Address'] + '> de MS-OutlookExpress'), 'express'+str(i)] )	
					#self.tree_options.append(mthun, [account['Account Name'], None, None, _('Importar cuenta de correo de MS-OutlookExpress a Thunderbird'), 'oue2thun'+str(i)] )	
			if n_ou:
				account = self.selected_user.get_OUTLOOK_account(i)
				if account: n_ou = n_ou -1
				if account and 'Account Name' in account.keys():
					self.tree_options.append(ou, [account['Account Name'], None, None, _('Cuenta de correo <' + account['SMTP Email Address'] + '> de MS-Outlook'), 'outlook'+str(i)] )
					#self.tree_options.append(mthun, [account['Account Name'], None, None, _('Importar cuenta de correo de MS-Outlook a Thunderbird'), 'ou2thun'+str(i)] )
			seguir_buscando = (n_ou or n_oue) and (i<20)
			
		# Bookmarks
		self.tree_options.append(parent, [_("IExplorer"), None, None, _('Favoritos de Internet Explorer'), 'iexplorer'] )
		if os.path.exists(os.path.join(self.selected_user.folders['AppData'].path, 'Opera', 'Opera', 'profile', 'opera6.adr')):
			self.tree_options.append(parent, [_("Opera"), None, None, _('Marcadores de Opera'), 'opera'] )
		if os.path.exists(os.path.join(self.selected_user.folders['AppData'].path, 'Mozilla', 'Firefox')):
			self.tree_options.append(parent, [_("Mozilla-Firefox"), None, None, _('Marcadores de Mozilla-Firefox'), 'firefox'] )
		# Instant Messenger
		messenger = self.selected_user.get_messenger_accounts()
		msn = self.selected_user.get_MSN_account()
		yahoo = self.selected_user.get_YAHOO_account()
		gtalk = self.selected_user.get_GTALK_account()
		aol = self.selected_user.get_AOL_accounts()
		if msn or yahoo or gtalk or aol or messenger:
			im = self.tree_options.append(parent, [_("Mensajeria Instantanea"), None, None, _('Cuentas de IM'), None] )
		for m in messenger:
			self.tree_options.append(im, [m, None, None, _('Cuenta de MSN Messenger'), m] )
		if msn:
			self.tree_options.append(im, [msn, None, None, _('Cuenta de MSN Messenger'), 'msn'] )
		if yahoo:
			self.tree_options.append(im, [yahoo, None, None, _('Cuenta de Yahoo! Messenger'), 'yahoo'] )
		if gtalk:
			self.tree_options.append(im, [gtalk, None, None, _('Cuenta de Gtalk'), 'gtalk'] )
		for a in aol:
			self.tree_options.append(im, [a, None, None, _('Cuenta de American On-Line'), None] )
			
		# eMule Config	
		if 'eMule' in self.selected_user.folders.keys():
			self.tree_options.append(parent, [_("eMule"), None, None, _('Configuracion del eMule'), 'emule'] )

		#Fonts
		if self.selected_user.folders['Fonts'].path:
			self.tree_options.append(parent, [_("Fuentes"), None, str(self.selected_user.folders['Fonts'].get_size()), _('Fuentes True-Type'), 'fuentes'] )

		#Wallpaper
		if 'Wallpaper' in self.selected_user.folders.keys():
			self.tree_options.append(parent, [_("Fondo de Escritorio"), None, None, _('Imagen de fondo de escritorio'), 'wallpaper'] )
		print _("Opciones de migración generadas")
		
	def buscar_usuarios(self):
		"""Busca usuarios de Windows 2000/XP en el ordenador"""
		pc = computer.pc()
		wusers = pc.get_win_users()
		options = {}
		if not wusers:
			print _('Intentando montar particiones...')
			pc.mount_win_parts()
			wusers = pc.search_win_users()
		if not wusers:
			self.dialogo_advertencia(_('No se han detectado usuarios de Windows en el ordenador.') + '\n' + _('Revise que todas las unidades de disco esten montadas.'))
			return 0
		aviso = ""
		for u in wusers:
			usuario = computer.user(u)
			if usuario.folders:
				self.usuarios[usuario.get_name()] = usuario
				self.combo.append_text(usuario.get_name())
			else:
				aviso = _("No se pudo acceder a algunos de los usuarios de Windows.") + "\n" + _("Por favor revise que las particiones estén montadas y que dispone de permisos de lectura sobre ellas")
		self.textbuffer.set_text( str(len(self.usuarios)) +' ' + _("usuario(s) encontrado(s)") + "\n" + aviso)


	def main(self):
		gtk.main()
		
	
	def generar_vista( self, model ):
		"""Representa el arbol de opciones de migración"""
		self.view = gtk.TreeView( model )
		# setup the text cell renderer and allows these
		# cells to be edited.
		self.renderer = gtk.CellRendererText()
	
		# The toggle cellrenderer is setup and we allow it to be
		# changed (toggled) by the user.
		self.renderer1 = gtk.CellRendererToggle()
		self.renderer1.set_property('activatable', True)
		self.renderer1.connect( 'toggled', self.marcar_opcion, model )
		
		# Connect column0 of the display with column 0 in our list model
		# The renderer will then display whatever is in column 0 of
		# our model .
		self.column0 = gtk.TreeViewColumn(_("Opción"), self.renderer, text=0)
		
		self.column2 = gtk.TreeViewColumn(_("Tamaño (KB)"), self.renderer, text=2)
		self.column3 = gtk.TreeViewColumn(_("Descripción"), self.renderer, text=3)
		# The columns active state is attached to the second column
		# in the model. So when the model says True then the button
		# will show as active e.g on.
		self.column1 = gtk.TreeViewColumn(_("Migrar"), self.renderer1 )
		self.column1.add_attribute( self.renderer1, "active", 1)
		self.view.append_column( self.column0 )
		self.view.append_column( self.column1 )
		self.view.append_column( self.column2 )
		self.view.append_column( self.column3 )
		self.view.expand_all()
		return self.view
	
	def generar_resumen(self, model):
		"""Genera el resumen de las tareas pendiente"""
		if self.resumen:
			res = _('Los siguientes archivos y/o configuraciones serán importados') + ':\n'
			self.boton3.set_sensitive(True)
		else:
			res = _('No se han encontrado tareas que realizar')
			# Don't allow to continue
			self.boton2.set_sensitive(False)
			self.boton3.set_sensitive(False)
		for r in self.resumen:
			res += '\t'+ r + '\n'
		self.textbuffer1.set_text(res)
	
	
	def recorrer_arbol(self, iterator, model, checked):
		"""Establece el valor marcado/desmarcado del padre a las opciones hijas"""
		changed = (checked != model.get_value(iterator, 1))
		#ponemos el valor a la raiz
		model.set_value(iterator, 1, checked)
		# modificamos los parametros si hay algun cambio
		if changed:
			model.set_value(iterator, 1, checked)
			tam = model.get_value(iterator, 2)
			res = model.get_value(iterator, 3)
			task = model.get_value(iterator, 4)
			if checked:
				if tam: self.required_space = self.required_space + int(tam)
				if task: 
					self.tasks.append(task)
					self.resumen.append(res)
			else:
				if tam: self.required_space = self.required_space - int(tam)
				if task: 
					self.tasks.remove(task)
					self.resumen.remove(res)
		# si tiene filas hijas
		if model.iter_children(iterator):
			hijas = model.iter_children(iterator)
			while hijas:
				self.recorrer_arbol(hijas, model, checked)
				# siguiente elemento
				hijas = model.iter_next(hijas)


	def aplicar(self):
		"""Comienza el proceso de migración"""
		self.pipe_r, self.pipe_w = os.pipe()
		pid = os.fork()
		if not pid: # child porcess
			os.close(self.pipe_r)
			detalles = _("Detalles de la migración:\n")
			for task in self.tasks:
				now = time.strftime(" %H:%M:%S", time.gmtime())
				try:
					detalles += "%s >>" % (now) + self.tarea(task)
				except:
					detalles += now + ' >>\t' + _('Fallo en la tarea ') + task + '\n'
			print _('Migración finalizada')
			if self.selected_user.errors:
				mensaje = _("Migración concluida con errores:\n\n\n")
				for e in self.selected_user.all_errors:
					detalles += e + '\n'
				
			else:
				 detalles += _("Migración concluida satisfactoriamente") + "\n\n\n"
			time.sleep(0.5)
			os.write(self.pipe_w, _('Finalizado'))
			time.sleep(0.5)
			os.write(self.pipe_w, detalles)
			time.sleep(0.5)
			os._exit(1)
		else: # father process
			os.close(self.pipe_w)
			msg = ''
			n_tasks = len(self.tasks)
			b = barra_progreso(n_tasks, self.pipe_r)
			b.start()
			self.boton.hide()
			self.boton4.show_all()
			
			
	def tarea(self, task):
		"""Ejecuta la tarea seleccionada
		
		Returns String"""
		
		if task == 'documentos':
			# copy Personal (My documents) folder
			mensaje_progreso =_('Copiando archivos personales...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			self.selected_user.folders['Personal'].copy(self.destino, exclude = ['.lnk','.ini'])
			return ( _("\tCopiados Documentos personales\n"))
			
		elif task == 'escritorio':
			# copy Desktop folder
			mensaje_progreso = _('Copiando archivos personales...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			escritorio = folder(os.path.join(self.destino, 'Desktop'))
			self.selected_user.folders['Desktop'].copy(escritorio.path, exclude = ['.lnk','.ini'])
			return ( _("\tCopiados Documentos del escritorio\n"))

		elif task == 'temporales':

			mensaje_progreso= _('Copiando descargas incompletas...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			self.selected_user.import_eMule_temp()
			return ( _("\tCopiados Archivos temporales\n"))
			
		elif task == 'descargados':
		
			mensaje_progreso = _('Copiando descargas finalizadas...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			self.selected_user.import_eMule_files()
			return ( _("\tCopiados Archivos descargados\n"))
			
		elif task == 'wallpaper':
			
			mensaje_progreso = _('Configurando fondo de escritorio...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			settings.set_wallpaper(self.selected_user.folders['Wallpaper'])
			return ( _("\tInstalado el fondo de escritorio\n"))
			
		elif task == 'fuentes':
			
			mensaje_progreso = _('Copiando fuentes tipograficas...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			settings.import_fonts(self.selected_user.folders['Fonts'])
			return ( _("\tCopiadas fuentes tipograficas\n"))
			
		elif task == 'emule':
			
			mensaje_progreso = _('Importando configuracion del eMule...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			self.selected_user.config_aMule()
			return (_( "\tImportada configuracion del eMule\n"))
			
		elif task == 'iexplorer':
			
			mensaje_progreso = _('Importando favoritos de Internet Explorer...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			f = webbrowser.firefox(self.selected_user.folders)
			f.import_favorites_IE()
			return ( _("\tImportados favoritos de Internet Explorer\n"))
			
		elif task == 'opera':
			
			mensaje_progreso = _('Importando marcadores de Opera...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			f = webbrowser.firefox(self.selected_user.folders)
			f.import_bookmarks_opera()
			return (_( "\tImportados marcadores de Opera\n"))
			
		elif task == 'firefox':
			
			mensaje_progreso = _('Importando marcadores de Firefox...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			f = webbrowser.firefox(self.selected_user.folders)
			f.import_bookmarks_firefox()
			return ( _("\tImportados marcadores de Firefox\n"))
			
		elif task == 'msn':
			
			mensaje_progreso = _('Configurando cuentas de mensajeria instantanea...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			msn = self.selected_user.get_MSN_account()
			gaim = messenger.gaim()
			gaim.config_msn(msn)
			messenger.msn2amsn(msn)
			return ( _("\tConfigurada cuenta de MSN\n"))
			
		elif task == 'yahoo':
			
			mensaje_progreso = _('Configurando cuentas de mensajeria instantanea...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			yahoo = self.selected_user.get_YAHOO_account()
			gaim = messenger.gaim()
			gaim.config_yahoo(yahoo)
			return ( _("\tConfigurada cuenta de Yahoo!\n"))
			
		elif task == 'gtalk':
			
			mensaje_progreso = _('Configurando cuentas de mensajeria instantanea...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			gtalk = self.selected_user.get_GTALK_account()
			gaim = messenger.gaim()
			gaim.config_gtalk(gtalk)
			return ( _("\tConfigurada cuenta de Gtalk\n"))
			
		elif task == 'thunderbird':
			
			mensaje_progreso = _('Importando configuracion de Mozilla-Thunderbird...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			t = mail.thunderbird()
			t.import_windows_settings(self.selected_user.folders['AppData'].path)
			return ( _("\tImportada configuracion de Mozilla-Thunderbird\n"))
			
		elif task.find('express') != -1:
			
			mensaje_progreso = _('Importando cuenta de correo de Outlook Express...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			account = self.selected_user.get_OUTLOOKexpress_account(int(task.replace('express', '')))
			if account:
				mail.config_EVOLUTION(account)
				t = mail.thunderbird()
				t.config_account(account)
				return (_( "\tImportada cuenta de Outlook-Express\n"))
			
		elif task.find('outlook') != -1:
			
			mensaje_progreso = _('Importando cuenta de correo de Outlook...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			account = self.selected_user.get_OUTLOOK_account(int(task.replace('outlook', '')))
			if account:
				mail.config_EVOLUTION(account)
				t = mail.thunderbird()
				t.config_account(account)
				return ( _("\tImportada cuenta de Outlook\n"))
			
		elif task.find('oumails') != -1:
			mensaje_progreso = _('Importando correos de Outlook...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			aviso = ""
			for b in self.buzones_ou:
				if mail.convert_pst(b):
					aviso = _("AVISO: Se ha detectado que alguno de sus buzones de correo están en formato de Outlook 2003.") + '\n ' + _("Debido a incompatibilidades con dicho formato no se han podido importar dichos correos.") + '\n ' + _("Por favor, visite la página web de Amigu para ver las alternativas posibles. Disculpen las molestias") + '\n'
					try:
						shutil.copy2('/usr/share/amigu/AMIGU y Outlook 2003.url', self.selected_user.folders['Desktop'].path)
					except:
						pass
			return '\t' + _('Importados correos de Outlook') + '\n ' + aviso
			
		elif task.find('ouemails') != -1:
			mensaje_progreso = _('Importando correos de Outlook Express...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			for b in self.buzones_oue:
				if b.find('Folders.dbx') != -1:
					mail.convert_dbx(b)
			return ( _("\tImportados correos de Outlook Express\n"))
				
		elif task.find('@') != -1:
			
			mensaje_progreso = _('Configurando cuentas de mensajeria instantanea...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			gaim = messenger.gaim()
			gaim.config_msn(task)
			messenger.msn2amsn(task)
			return ( _("\tConfigurada cuenta de MSN\n"))
		
		elif task.find('2thun') != -1:
			
			mensaje_progreso = _('Importando cuentas de correo a Thunderbird...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			t = mail.thunderbird()
			account = None
			if task.find('oue2') != -1:
				account = self.selected_user.get_OUTLOOKexpress_account(int(task.replace('oue2thun', '')))
			elif task.find('ou2') != -1:
				account = self.selected_user.get_OUTLOOK_account(int(task.replace('ou2thun', '')))
			if account:
				t.config_account(account)
				return ( _("\tConfigurada cuenta de correo en Thunderbird\n"))
		
		elif task == 'oueaddressbook':
			
			mensaje_progreso = _('Importando libreta de direcciones...')
			os.write(self.pipe_w, mensaje_progreso)
			time.sleep(0.5)
			adlist = mail.get_outlook_addressbook(self.adbook)
			mail.generate_csv(adlist)
			mail.generate_html(adlist)
			t = mail.thunderbird()
			t.generate_impab(adlist)
			return ( _("\tImportada libreta de direcciones\n"))

if __name__ == "__main__":
 print _("Asistente de MIgración de Guadalinex")
 print '(C) 2006-2007 Fernando Ruiz Humanes, Emilia Abad Sanchez\nThis program is freely redistributable under the terms\nof the GNU General Public License.\n'
 base = Asistente()
 base.main()
