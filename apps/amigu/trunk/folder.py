#!/usr/bin/python
# -*- coding: utf-8 -*-
# Este módulo agrupa rutinas para el tratamiento de archivos y carpetas.
# Ofrece la funcionalidad de realizar backup's de archivos y restaurarlos posteriormente.
# Permite le uso de objetos de tipo 'folder' para simplificar y agilizar
# el manejo de las carpetas

import os, re, shutil

def backup(file):
	"""Crea una copia de respaldo del archivo que se le indica"""
	if os.path.exists(file):
		try:
			progress('Doing backup of ' + file)
			shutil.copy2(file, file + '.bak')
			return file + '.bak'
		except (IOError, os.error), why:
			error("Can't backup %s: %s" % (file, str(why)))

	
def restore_backup(backup):
	"""Restaura la copia de respaldo del archivo que se le indica"""
	if os.path.exists(backup):
		try:
			progress('Restoring backup of ' + backup[:-4])
			shutil.copy2(backup, backup[:-4])
			return 1
		except (IOError, os.error), why:
			error("Can't restore backup %s: %s" % (backup, str(why)))

def error(e):
	"""Error handler"""
	print e
	
def warning(w):
	"""Show warnings"""
	print w
	
def progress(p):
	"""Show the progress"""
	print p

class folder:
	"""Clase para el manejo de carpetas y archivos"""
	#
	# rutas por defecto
	#
	__DIR_BACKUP__="/tmp/migration-assistant"
	__DIR_MUSIC__='~/Music'
	__DIR_PICTURES__='~/Pictures'
	__DIR_VIDEO__='~/Video'
	#
	# extensiones
	#
	texto = ['.sxv', '.stw', '.doc', '.rtf', '.sdw', '.var', '.txt', '.html', '.htm', '.pdf']
	calculo = ['.sxc', '.stc', '.dif', '.dbf', '.xls', '.xlw', '.xlt', '.sdc', '.vor', '.slk', '.csv', '.txt', '.html', '.htm']
	presentacion = ['.sxi', '.sti', '.ppt', '.pps', '.pot', '.sxd', '.sda', '.sdd', '.vor', '.pdf']
	pictures = ['.jpg', '.jpeg', '.png', '.bmp', '.gif', '.pdf', '.ico', '.tif', '.tiff']
	audio = ['.wma', '.asf', '.wav', '.mp2', '.mp3', '.aac', '.m4a', '.ogg', '.mp4', '.mid', '.midi', '.dts', '.ac3']
	video = ['.avi', '.mpg', '.mpeg', '.divx', '.mov', '.mp4', '.vob', '.ifo', '.bup', '.wmv', '.3gp', '.ogm', '.mkv', '.rm']
	compresion = ['.zip', '.rar', '.r*' , '.7z', '.cab', '.tar', '.gz' , '.bz', '.bz2', '.ace', '.arj', '.z', '.cpio', '.rpm', '.deb', '.lzma', '.rz', '.arc', '.alz', '.arj', '.zz', '.tar.gz', '.tgz', '.tar.Z', '.tar.bz2', '.tbz2']
	ejecutable = ['.exe']
	edonkey = ['.dat', '.part', '.part.met', '.met']
	
	def __init__(self, path, create = True):
		"""Constructor de la clase.
		Recibe una ruta y si no existe la crea"""
		self.errors = []
		self.path = None
		if os.path.exists(path) and os.path.isdir(path):
			self.path = path
		elif not os.path.exists(path) and create:
			self.path = self.create_folder(path)
		if self.errors: 
			print self.errors
			#self.path = path

	def error(self, e):
		"""Almacena los errores en tiempo de ejecución"""
		self.errors.append(e)
		
	def get_path(self):
		"""Devuelve la ruta del objeto"""
		return self.path
		
	def is_writable(self):
		"""Devuelve si tiene permisos de escritura sobre la carpeta"""
		return os.access(self.path, os.W_OK)
	
	def is_readable(self):
		"""Devuelve si tiene permisos de lectura sobre la carpeta"""
		return os.access(self.path, os.R_OK)
		
	def get_info(self):
		"""Devuelve información con el número de archivos y carpetas y el tamaño del objeto"""
		dirs, files = self.count(self.path, 0 , 0)
		size = self.get_size()
		if size > 1024*1024:
			size = str(size/(1024*1024)) + 'GB'
		elif size > 1024:
			size = str(size/1024) + 'MB'
		else:
			size = str(size) + 'KB'
		return '%d archivos en %d carpetas ocupando %s en disco' % (files, dirs, size)
	
	def get_size(self):
		"""Devuelve el tamaño de la carpeta"""
		if self.path:
			try:
				tam = os.popen('du -s ' + self.path.replace(' ','\ '))
				return int(tam.read().split('\t')[0])
			except:
				self.error('Size for %s not available' % self.path)
				return 0
		else:
			return 0
			
	def count(self, path, dirs, files):
		"""Devuelve el número de archivos y carpetas de forma recursiva"""
		for e in os.listdir(path):
			if os.path.isdir(os.path.join(path, e)):
				dirs, files = self.count(os.path.join(path, e), dirs, files)
				dirs = dirs + 1
			elif os.path.isfile(os.path.join(path, e)):
				files = files + 1
		return dirs, files
			
	def get_free_space(self):
		"""Devuelve el espacio libre en la carpeta"""
		try:
			df = os.popen('df ' + self.path.replace(' ','\ '))
		except:
			self.error ("Free space not available")
		else:
			df.readline()
			file_sys,disc_size,disc_used,disc_avail,disc_cap_pct,mount=df.readline().split()
			df.close()
			return int(disc_avail)
	
	def copy(self, destino, extension = None, reorder = None, exclude = ['.lnk',]):
		"""Copia solo los archivos de la ruta origen que tienen la misma extension que la especificada en la lista de 'extensiones'"""
		for e in os.listdir(self.path):
			ruta = os.path.join(self.path, e)
			try:
				if os.path.isdir(ruta):
					# caso recursivo
					suborigen = folder(ruta)
					subdestino = self.create_folder(os.path.join(destino, e))
					if suborigen and subdestino:
						suborigen.copy(subdestino, extension, reorder)
				elif os.path.isfile(ruta):
					# caso base
					ext = os.path.splitext(e)[1]
					if (not exclude or (not ext in exclude)) and (not extension or (ext in extension)): # think about some file to exclude, like .ini o .lnk
						try:
							progress(">>> Copying %s..." % ruta)
							if reorder and (ext in audio):
								shutil.copy2(ruta, __DIR_MUSIC__)
							elif reorder and (ext in pictures):
								shutil.copy2(ruta, __DIR_PICTURES__)
							elif reorder and (ext in video):
								shutil.copy2(ruta, __DIR_VIDEO__)
							else:
								shutil.copy2(ruta, destino)
							os.chmod(os.path.join(destino, e), 0644)
						except:
							self.error('Imposible copiar ' + e)
				else:
					# Tipo desconocido, se omite
					self.error('Skipping %s' % ruta)
			except:
				self.error('Failed Stat over ' + e)

	def search_by_ext(self, extension):
		"""Devuelve una lista de archivos que cumplen con la extension dada"""
		found = []
		for e in os.listdir(self.path):
			ruta = os.path.join(self.path, e)
			try:
				if os.path.isdir(ruta):
					# caso recursivo
					suborigen = folder(ruta)
					if suborigen:
						found += suborigen.search_by_ext(extension)
				elif os.path.isfile(ruta):
					# caso base
					ext = os.path.splitext(e)[1]
					if (ext == extension ) :
						#print ruta
						found.append(ruta)
				else:
					# Tipo desconocido, se omite
					self.error('Skipping %s' % ruta)
			except:
				self.error('Failed Stat over ' + e)
		return found
				
	def create_folder(self, path):
		"""Crea la carpeta"""
		try:
			os.makedirs(path)
			return path
		except OSError, e:
			if re.search('Errno 17',str(e)): # if already exists
				return path
			elif re.search('Errno 30',str(e)): # only read path
				self.error('No se puede escribir en ' + path)
			elif re.search('Errno 13',str(e)): # permission denied
				self.error ('Permiso denegado para escribir en ' + path)
			else: # invalid path
				self.error ('Ruta no valida: ' + os.path.join(path, folder))
	
	def create_subfolder(self, subfolder):
		"""Crea una subcarpeta en la ruta del objeto"""
		try:
			os.makedirs(os.path.join(self.path, subfolder))
			return os.path.join(self.path, subfolder)
		except OSError, e:
			if re.search('Errno 17',str(e)): # if already exists
				return os.path.join(self.path, subfolder)
			elif re.search('Errno 30',str(e)): # only read path
				self.error('No se puede escribir en ' + self.path)
			elif re.search('Errno 13',str(e)): # permission denied
				self.error ('Permiso denegado para escribir en ' + self.path)
			else: # invalid path
				self.error ('Ruta no valida: ' + os.path.join(self.path, subfolder))

def search_folder(folder):
	"""Busca la carpeta solicitada que pueda haber en las particiones vfat y ntfs"""
	win_parts = search_win_parts()
	for part in win_parts:
		if  os.path.exists(os.path.join(part, folder)):
			return part
		
def search_win_units(reg):
	"""Asocia los puntos de montaje de Linux con la asiganción de unidades de Windows.
	Recibe información obtenida del registro"""
	units = {}
	win_parts = search_win_parts()
	for k, v in reg.iteritems():
		c = v.split('/')
		if c[0] and not (c[0] in units.keys()):
			for part in win_parts:
				if  c[1] and os.path.exists(os.path.join(part, c[1])):
					units[c[0]]= part
	return units
			
def search_win_parts():
	"""Busca particiones Windows (ntfs y vfat) montadas en el sistema y devuelve los puntos de montaje"""
	try:
		f = open("/etc/mtab",'r')
	except:
		error('No se puede determinar las unidades montadas')
	else:
		win_parts = []
		l = re.compile('\s*(?P<dev>[/\-\w]+)\s+(?P<mountpoint>/[/\-\w]+)\s+(?P<type>\w+)\s+\.*')
		for entrada in f.read().split('\n'):
			disk = l.match(entrada)
			try:
				if disk.group('type') == 'fuseblk' or disk.group('type') == 'ntfs' or disk.group('type') == 'vfat':
					win_parts.append(disk.group('mountpoint'))
			except:
				disk = None
		f.close()
		return win_parts

if __name__ == "__main__":
    f = folder('/media/hda2')
    print f.get_info()
    print 'freespace: %dKB' % f.get_free_space()
    print f.search_by_ext('.txt')
