#!/usr/bin/python
# -*- coding: utf-8 -*-
# La clase registry permite acceder a la información contenida en el registro de Windows.
# La compatibilidad con los registros de Windows viene dada por el programa 'dumphive'
# que convierte el contenido del registro en un fichero de texto plano.
# Se añaden funcionalidades para buscar claves prefijadas dentro del registro de usuario
# o NTUSER.DAT asi cómo la función genérica de buscar cualquier entrada del registro.
import os, re, codecs, tempfile
from folder import *

class winregistry:
	"""Clase para el registro de Windows"""
	__dumphive="/usr/bin/dumphive"

	def __init__(self, path):
		"""Constructor de la clase.
		Recibe la ruta de un directorio que contenga un archivo del registro de Windows"""
		self.tempreg = tempfile.mktemp()
		#self.user_reg = tempfile.mktemp()
		#self.system_reg = tempfile.mktemp()
		self.errors = []
		ntuser1 = os.path.join(path, 'NTUSER.DAT')
		ntuser2 = os.path.join(path, 'ntuser.dat') # for case sensistives filesystems
		system = os.path.join(path, 'WINDOWS','system32','config','system')
		if os.path.exists(self.__dumphive):
			if os.path.exists(ntuser1):
				try:
					os.system("%s %s %s" % (self.__dumphive, ntuser1.replace(" ","\ "), self.tempreg))
				except:
					self.errors.append('Failed to read user registry')
			elif os.path.exists(ntuser2):
				try:
					os.system("%s %s %s" % (self.__dumphive, ntuser2.replace(" ","\ "), self.tempreg))
				except:
					self.errors.append('Failed to read user registry')
			elif os.path.exists(system):
				try:	
					os.system("%s %s %s" % (self.__dumphive, system.replace(" ","\ "), self.tempreg))
				except:
					self.errors.append('Failed to read system registry')
			else:
				self.error("Invalid path")
	
	def error (self, e):
		"""Alamacena los errores en tiempo de ejecución"""
		self.errors.append(e)
	
	def search_key(self, key):
		"""Busca la clave en el registro de Windows.
		Devuelve un diccionario con los valores de la clave"""
		try:
			reg = codecs.open(self.tempreg, 'r', 'iso-8859-1')
		except:
			self.error('No fue posible acceder al archivo')
		else:
			entrada = "a"
			res = {}
			while not res and entrada != "":
				entrada = reg.readline()
				if entrada.find(key) != -1:
					while entrada != "\r\n":
						entrada = reg.readline()
						if entrada != "\r\n":
							#entrada = entrada.encode('utf-8')
							entrada = entrada.replace('\\\\','/')
							entrada = entrada.replace('\"','')
							try:
								separator = entrada.find('=')
								clave = entrada[:separator]
								valor = entrada[(separator+1):-2]
								res[clave]=valor
							except:
								self.error("error de lectura en el registro")
			reg.close()
			return res
	
	def get_MSN_account(self):
		"""Devuelve la cuenta asociada a MSNMessenger"""
		r = self.search_key("Software\Microsoft\MSNMessenger")
		try:
			return r["User .NET Messenger Service"]
		except:
			return 0
		
	def get_YAHOO_account(self):
		"""Devuelve el identificador usado en Yahoo! Messneger"""
		r = self.search_key("Software\Yahoo\pager")
		try:
			return r["Yahoo! User ID"]
		except:
			return 0
		
	def get_AOL_accounts(self):
		"""Devuelve una lista con los usuarios de AOL Instant Messenger"""
		r = self.search_key("Software\America Online\AOL Instant Messenger (TM)\CurrentVersion\Users")
		return r
		
	def get_GTALK_account(self):
		"""Devuelve el identificador usado en Gtalk"""
		r = self.search_key("Software\Google\Google Talk\Accounts")
		try:	
			return r['a'].split('/')[0]
		except:
			return 0
		
	def get_OUTLOOKexpress_account(self, num):
		"""Devuelve la configuración de la cuenta de OutlookExpress que se le indica"""
		num = 00000000 + num
		r = self.search_key("Software\Microsoft\Internet Account Manager\Accounts\%08d" % num)
		return r
		
	def get_OUTLOOK_account(self, num):
		"""Devuelve la configuración de la cuenta de Outlook que se le indica"""
		num = 00000000 + num
		r = self.search_key("Software\Microsoft\Office\Outlook\OMI Account Manager\Accounts\%08d" % num)
		return r
		
	def num_OUTLOOK_accounts(self):
		"""Devuelve el número de cuentas de Outlook y OutlookExpress (por ese orden) que están almacenadas en el registro"""
		try:
			reg = open(self.tempreg, 'r')
		except:
			self.error('No fue posible acceder al archivo')
		else:
			ou, oue = 0, 0
			for e in reg.readlines():
				if e.find('\Software\\Microsoft\\Office\\Outlook\\OMI Account Manager\\Accounts\\0') != -1:
					ou = ou + 1
				elif e.find('\Software\\Microsoft\\Internet Account Manager\\Accounts\\0') != -1:
					oue = oue + 1
			reg.close()
			return (ou, oue)
					
	def get_user_folders(self):
		"""Devuelve las carpetas de archivos, confifguraciones y programas del usuario"""
		r = self.search_key("Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders")
		e = self.search_key("Software\eMule")
		w =  self.search_key("Control Panel\Desktop")
		if e:
			r["eMule"] = e["Install Path"]
		if w:
			r["Wallpaper"] = w["Wallpaper"]
		if r:
			self.mount_points = search_win_units(r)
			print "\nGetting shell folders..."
			for k, v in r.iteritems():
				if v:
					unit = v[:2]
					try:
						if not k == 'Wallpaper':
							r[k] = folder(v.replace(unit, self.mount_points[unit]), False)
							print "%s: %s --> %s" % (k, v, r[k].path)
						else:
							r[k] = v.replace(unit, self.mount_points[unit])
							print "%s: %s --> %s" % (k, v, r[k])
						
					except:
						r = 0
						print "Unknown partition: %s" % unit
						break
			print "\n"		
			return r
	
	def clean (self):
		"""Borra el archivo temporal del registro"""
		try:
			os.system('rm %s' % self.tempreg)
			#os.system('rm %s' % self.user_reg)
			#os.system('rm %s' % self.system_reg)
		except:
			self.error("Can't erase tempfiles")
			
# end class winregistry

############################################################################

if __name__ == "__main__":
	reg = winregistry('/media/hda2/Documents and Settings/mjose')
	print reg.errors
	print reg.get_user_folders()
	reg.clean()
