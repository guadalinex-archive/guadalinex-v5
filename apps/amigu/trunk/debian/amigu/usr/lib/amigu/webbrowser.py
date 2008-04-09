#!/usr/bin/env python
# -*- coding: UTF-8 -*-
import os, re, shutil, computer
from folder import *


class webbrowser:
	"""Clase generica para los distintos navegadores"""
	
	def __init__(self, folders):
		"""Contructor de la clase.
		Recibe un diccionario con las rutas obtenidas del registro"""
		self.favoritesIE = folders['Favorites']
		self.appdata = folders['AppData']
		self.report = {'opera':'not imported yet','firefox':'not imported yet','iexplorer':'not imported yet'}
		self.errors = []
		
	def get_ie_favorites(self):
		"""Devuelve la ruta donde se almacenan los Favoritos de Internet Explorer"""
		return favoritesIE
	
	def get_opera_bookmarks(self):
		"""Devuelve una lista con los Marcadores de Opera"""
		opera = os.path.join(self.appdata.path,'Opera','Opera','profile','opera6.adr')
		if os.path.exists(opera):
			try:
				marks = open(opera,'r')
			except:
				self.error("Fallo al abrir los marcadores de Opera")
			else:
				hotlist = marks.read().split('#')
				marks.close()
				links = []
				dict = None
				for h in hotlist:
					e = h.split('\r\n')
					if e[0]=="FOLDER":
						if dict:
							links.append(dict)
						folder=e[2].replace('\t','')[5:]
						dict = {folder:[]}
					elif e[0]=="URL":
						name=e[2].replace('\t','')[5:]
						url=e[3].replace('\t','')[4:]
						dict[folder].append({name: url})
				return links
	
	def get_firefox_bookmarks(self, os_type):
		"""Devuelve la ruta del fichero Bookmarks.html de Firefox. 
		Es preciso especificar el tipo de sistema operativo: 'windows' o 'linux'"""
		if os_type == "windows":
			firefox = os.path.join(self.appdata.path,'Mozilla','Firefox')
		if os_type == "linux":
			firefox = os.path.join(os.path.expanduser('~'),'.mozilla','firefox')
		profile = self.get_firefox_profile(firefox)
		if profile:
			bookmarks = os.path.join(profile, 'bookmarks.html')
			return os.path.exists(bookmarks) and bookmarks or None
			
	def get_firefox_profile(self, firefox):
		"""Devuelve el perfil actual de Firefox. En caso de no existir crea uno nuevo"""
		profiles_ini = os.path.join(firefox,'profiles.ini')
		if os.path.exists(profiles_ini):
			try:
				prof = open(profiles_ini, 'r')
			except:
				self.error ('Unable to read Firefox profiles')
			else:
				relative = 0
				for e in prof.read().split('\n'):
					if re.search("IsRelative=1",e):
						relative = 1
					if re.search("Path",e):
						profile = e.split('=')[1]
				prof.close()
				if relative:
					path_profile = os.path.join(firefox, profile)
				else:
					path_profile = profile
				if path_profile[-1]=='\r':
					return path_profile[:-1]
				else:
					return path_profile
	
	def error (self, e):
		"""Almacena los errores en tiempo de ejecución"""
		self.errors.append(e)
		
	### Metodos abstractos ###
	def import_favorites_IE(self):
		"""Importa los favoritos de Internet Explorer"""
		pass
	
	def add_favorites(self, orig, tab, fd):
		"""Recorrido recursivo sobre las carpetas de favoritos para añadirlos a la configuración.
		Recibe el elemento actual, el nivel de tabulación y el descriptor del fichero de escritura"""
		pass
	
	def import_bookmarks_firefox(self):
		"""Importa los marcadores de Mozilla-Firefox"""
		pass
		
	def import_bookmarks_opera(self):
		"""Importa los marcadores de Opera"""
		pass
	
#end class browser

class konqueror(webbrowser):
	"""Clase para el navegador Konqueror de KDE (NO TERMINADA)"""
	
	def __init__(self,folders):
		"""Contructor de la clase.
		Recibe un diccionario con las rutas obtenidas del registro"""
		webbrowser.__init__(self, folders)
		konqueror = folder(os.path.join(os.path.expanduser('~'),'.kde','share','apps','konqueror'))
		self.bookmarks_file = os.path.join(konqueror.path,'bookmarks.xml')
		#create bookmark.xml if not exists
		if not os.path.exists(self.bookmarks_file):
			try:
				b = open(self.bookmarks_file, 'w')
				b.write ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE xbel>\n<xbel>\n</xbel>")
				b.close()
			except:
				self.error("Unable to create bookmarks.xml for Konqueror")

	def import_favorites_IE(self):
		"""Importa los favoritos de Internet Explorer"""
		if backup(self.bookmarks_file):
			try:
				b = open(self.bookmarks_file, 'w')
				o = open(self.bookmarks_file + '.bak',"r")
			except:
				b.close()
				o.close()
				self.report['iexplorer'] = 'Failed to modify bookmarks.xml'
				restore_backup(self.bookmarks_file + '.bak')
			else:
				# copiar los marcadores 
				for linea in o.readlines():
					if re.search('</xbel>', linea):
						b.write(' <folder>\n')
						b.write('  <title>Favoritos de IExplorer</title>\n')
						self.add_favorites(self.favoritesIE.path, "   ", b)
						b.write(' </folder>\n')
					b.write(linea)
				self.report['iexplorer'] = 'Imported Favorites from IExplorer successfully'
				b.close()
				o.close()

	def add_favorites(self, orig, tab, fd):
		"""Recorrido recursivo sobre las carpetas de favoritos.
		Recibe el elemento actual, el nivel de tabulación y el descriptor del fichero de escritura"""
		for e in os.listdir(orig):
			ruta = os.path.join(orig, e)
			if os.path.exists(ruta) and os.path.isdir(ruta):
				# recursive case
				fd.write(tab + '<folder>\n')
				fd.write(tab + '<title>' + e + '</title>\n')
				self.add_favorites(ruta, tab + " ", fd)
				fd.write(tab + '</folder>\n')
			elif os.path.exists(ruta) and os.path.isfile(ruta):
				# base case
				(name, ext) = os.path.splitext(e)
				if ext == '.url':
					try:
						f = open(ruta, "r")
						for l in f.read().split("\n"):
							if re.search('URL=', l):
								href = l.replace('URL=','HREF=\"').replace('\r\n','\"')
								fd.write('<bookmark href="%s">\n' % l)
								fd.write('<title>%s</title>\n' % name)
								fd.write('</bookmark>\n')
					finally:
						f.close()
	
	def import_bookmarks_firefox(self):
		"""Importa los marcadores de Mozilla-Firefox"""
		win_bookmarks = self.get_firefox_bookmarks('windows')
		if backup(self.bookmarks_file):
			try:
				b = open(self.bookmarks_file, "w")
				orig = open(self.bookmarks_file + '.bak',"r")
				wb = open(win_bookmarks, "r")
			except:
				b.close()
				orig.close()
				wb.close()
				self.report['firefox'] = "Failed to modify bookmarks.xml"
				restore_backup(self.bookmarks_file + '.bak')
			else:
				folder = re.compile('.*<DT><H3.*>(?P<name>.+)</H3>')
				bmark = re.compile('.*<DT><A HREF=(?P<url>.+) .*>(?P<name>.+)</A>')
				tab = ' '
				for l in orig.readlines():
					if re.search('</xbel>', l):
						b.write('<folder>\n')
						b.write(tab + '<title>Marcadores importados de Mozilla Firefox</title>\n')
						for e in wb.readlines():
							res1 = folder.match(e)
							res2 = bmark.match(e)
							if res1:
								b.write(tab + '<folder>\n')
								tab = tab + ' '
								b.write(tab + '<title>%s</title>\n' % res1.group('name'))
							elif res2:
								b.write(tab + '<bookmark href=%s>\n' % res2.group('url'))
								b.write(tab + ' <title>%s</title>\n' % res2.group('name'))
								b.write(tab + '</bookmark>\n')
					b.write(l)
				self.report['firefox'] = 'Imported Bookmarks from Mozilla-Firefox successfully'
				b.close()
				wb.close()
				orig.close()
	
	def import_bookmarks_opera(self):
		"""Importa los marcadores de Opera"""
		opera_bookmarks = self.get_opera_bookmarks()
		if backup(self.bookmarks_file) and opera_bookmarks:
			try:
				b = open(self.bookmarks_file, 'w')
				o = open(self.bookmarks_file + '.bak',"r")
			except:
				b.close()
				o.close()
				self.report['opera'] = 'Failed to modify bookmarks.xml'
				# si falla se restaura la copia de seguridad
				restore_backup(self.bookmarks_file + '.bak')
			else:
				for l in o.readlines():
					if re.search('</xbel>', l):
						b.write('<folder>\n')
						b.write('<title>Marcadores importados de Opera</title>\n')
						tab = ' '
						for m in opera_bookmarks:
							for folder, list_links in m.iteritems():
								b.write(tab + '<folder>\n')
								tab = tab + ' '
								b.write(tab + '<title>%s</title>\n' % folder)
								for e in list_links:
									for name, url in e.iteritems():
										b.write(tab + '<bookmark href="%s">\n' % url)
										b.write(tab + ' <title>%s</title>\n' % name)
										b.write(tab +'</bookmark>\n')
								tab = tab[:-1]
								b.write(tab + '</folder>\n')
					b.write(l)
				self.report['opera'] = 'Imported bookmarks from Opera successfully'
				b.close()
				o.close()

# end class konqueror

############################################################################

class firefox(webbrowser):
	"""Clase para el navegador Mozilla-Firefox"""
	
	def __init__(self,folders):
		"""Contructor de la clase.
		Recibe un diccionario con las rutas obtenidas del registro"""
		webbrowser.__init__(self, folders)
		firefox = folder(os.path.join(os.path.expanduser('~'),'.mozilla','firefox'))
		if not os.path.exists(os.path.join(firefox.path, 'profiles.ini')) and firefox:
			try:
				prof = open(os.path.join(firefox.path,'profiles.ini'), 'w')
				prof.write("[General]\n")
				prof.write("StartWithLastProfile=1\n\n")
				prof.write("[Profile0]\n")
				prof.write("Name=migrated\n")
				prof.write("IsRelative=1\n")
				prof.write("Path=m1gra73d.default\n")
				prof.close()
				path_profile = firefox.create_subfolder("m1gra73d.default")
				shutil.copy('/etc/firefox/profile/bookmarks.html',path_profile)
			except:
				self.error("Unable to create Mozilla Firefox profile")
				return 0
		self.bookmarks_file = self.get_firefox_bookmarks('linux')
	
	def import_favorites_IE(self):
		"""Importa los favoritos de Internet Explorer"""
		if backup(self.bookmarks_file):
			try:
				b = open(self.bookmarks_file, 'w')
				o = open(self.bookmarks_file + '.bak',"r")
			except:
				b.close()
				o.close()
				self.error("Failed to modify bookmarks.html")
				self.report['iexplorer'] = 'Failed to import Favorites from MS Internet Explorer'
				# si falla se restaura la copia de seguridad
				restore_backup(self.bookmarks_file + '.bak')
			else:
				importado = None
				for linea in o.readlines():
					b.write(linea)
					if not importado and re.search('</DL><p>', linea):
						b.write('\n<!-- Marcadores generados automaticamente por el asistente de migracion -->\n')
						b.write('\t<HR>\n')
						b.write('<DT><H3>Favoritos importados de Internet Explorer</H3>\n')
						b.write('<DD>Marcadores importados de Microsoft Internet Explorer\n')
						b.write('\t<DL><p>\n')
						self.add_favorites(self.favoritesIE.path, "\t\t", b)
						b.write('\t</DL><p>\n')
						importado = 1
				self.report['iexplorer'] = "Imported Favorites from IExplorer successfully"
				b.close()
				o.close()
				
	def add_favorites(self, origen, tab, fd):
		"""Recorrido recursivo sobre las carpetas de favoritos.
		Recibe el elemento actual, el nivel de tabulación y el descriptor del fichero de escritura"""
		for e in os.listdir(origen):
			ruta = os.path.join(origen, e)
			if os.path.exists(ruta) and os.path.isdir(ruta):
				# caso recursivo
				fd.write(tab + '<DT><H3>' + e + '</H3>\n')
				fd.write(tab + '<DD>Favoritos importados de Internet Explorer\n')
				fd.write(tab + '\t<DL><p>\n')
				self.add_favorites(ruta, tab + "\t", fd)
				fd.write(tab + '\t</DL><p>\n')
			elif os.path.exists(ruta) and os.path.isfile(ruta):
				# caso base
				(nombre, ext) = os.path.splitext(e)
				if ext == '.url': #filtrar por extension
					try:
						f = open(ruta, "r")
						for linea in f.read().split("\n"):
							if re.search('URL=', linea):
								href = linea.replace('URL=','HREF=\"')
								fd.write("%s<DT><A %s>%s</A>\n" % (tab, href.replace('\r','\"'), nombre))
					finally:
						f.close()
	
	def import_bookmarks_firefox(self):
		"""Importa los marcadores de Mozilla-Firefox"""
		winbookmarks = self.get_firefox_bookmarks('windows')
		if winbookmarks and backup(self.bookmarks_file):
			try:
				bookmark = open(self.bookmarks_file, "w")
				original = open(self.bookmarks_file + '.bak',"r")
				origen = open(winbookmarks, "r")
			except:
				bookmark.close()
				original.close()
				origen.close()
				self.report['firefox'] = 'Failed to import bookmarks from Mozilla-Firefox'
				# si falla se restaura la copia de seguridad
				restore_backup(self.bookmarks_file + '.bak')
			else:
				copiar, importado = None, None
				# copiar los marcadores originales
				for linea in original.readlines():
					bookmark.write(linea)
					if not importado and re.search('</DL><p>', linea):
						bookmark.write('<!-- Marcadores generados automaticamente por el asistente de migracion -->\n')
						bookmark.write('\t<HR>\n')
						bookmark.write('<DT><H3>Marcadores importados de Mozilla Firefox</H3>\n')
						bookmark.write('<DD>Marcadores importados de la version de Firefox en Windows\n')
						for marcador in origen.readlines():
							if copiar or re.search('<DL><p>', marcador):
								bookmark.write(marcador)
								copiar = 1
						importado = 1
				self.report['firefox'] = 'Imported bookmarks from Mozilla-Firefox successfully'
				bookmark.close()
				original.close()
				origen.close()
		
	def import_bookmarks_opera(self):
		"""Importa los marcadores de Opera"""
		opera_bookmarks = self.get_opera_bookmarks()
		#hacer backup de los marcadores actuales
		if opera_bookmarks and backup(self.bookmarks_file):
			try:
				b = open(self.bookmarks_file, 'w')
				o = open(self.bookmarks_file + '.bak',"r")
			except:
				b.close()
				o.close()
				self.error("Failed to modify bookmarks.html")
				self.report['opera'] = 'Failed to import bookmarks from Opera'
				# si falla se restaura la copia de seguridad
				restore_backup(self.bookmarks_file + '.bak')
			else:
				importado = None
				# copiar los marcadores oes
				for linea in o.readlines():
					b.write(linea)
					if not importado and re.search('</DL><p>', linea):
						b.write('\n<!-- Marcadores generados automaticamente por el asistente de migracion -->\n')
						b.write('\t<HR>\n')
						b.write('\t<DT><H3>Marcadores importados de Opera</H3>\n')
						b.write('<DD>Marcadores importados de Opera\n')
						b.write('\t<DL><p>\n')
						#leer los marcadores de Opera y procesarlos
						for e in opera_bookmarks:
							for folder, list_links in e.iteritems():
								b.write('\t<DT><H3>%s</H3>\n' % folder)
								b.write('<DD>Marcadores importados de Opera\n')
								b.write('\t<DL><p>\n')
								for l in list_links:
									for name, url in l.iteritems():
										b.write('\t\t'+"<DT><A HREF=\"%s\">%s</A>\n" % (url, name))
								b.write('\t</DL><p>\n')
						importado = 1
						b.write('\t</DL><p>\n')
				self.report['opera'] = 'Imported bookmarks from Opera successfully'
				b.close()
				o.close()
	
# end class firefox
	
############################################################################

if __name__ == "__main__":
	usr = computer.user('/media/hda2/Documents and Settings/mjose')
	folders = usr.get_user_folders()
	if folders:
		k = konqueror(folders)
		f = firefox(folders)
		f.import_favorites_IE()
		f.import_bookmarks_firefox()
		f.import_bookmarks_opera()
		print k.errors
		print f.errors
	usr.clean()
