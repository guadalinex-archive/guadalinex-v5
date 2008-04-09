#!/usr/bin/python
# -*- coding: utf-8 -*-
import os, re, registry, copy
import mail, webbrowser, messenger, settings, computer
from folder import *


class pc:
	"""Clase para el manejo de particiones del PC"""
	
	def __init__(self):
		"""Constructor de la clase"""
		self.errors = []
		self.win_parts = self.search_win_parts()
		self.win_users = self.search_win_users()
	
		
	def error(self, e):
		"""Almacena los errores en tiempo de ejecución"""
		self.errors.append(e)
		
	def mount_win_parts(self):
		"""Busca particiones Windows (ntfs y vfat) no montadas en el sistema para intentar montarlas"""
		try:
			f = open("/etc/fstab",'r')
		except:
			self.error('No se puede determinar las unidades definidas')
		else:
			win_parts = []
			l = re.compile('\s*(?P<dev>[/=\w\-]+)\s+(?P<mountpoint>/[/\-\w]+)\s+(?P<type>[\-\w]+)\s+\.*')
			for entrada in f.read().split('\n'):
				disk = l.match(entrada)
				try:
					print disk.group('type')
					if disk.group('type').startswith('ntfs') or disk.group('type') == 'vfat':
						try:
							os.popen('gksu mount %s' % disk.group('mountpoint'))
							self.win_parts.append(disk.group('mountpoint'))
						except:
							self.error('Fallo al montar la partición')
				except:
					disk = None
			f.close()
		
	def search_win_parts(self):
		"""Busca particiones Windows montadas en el sistema.
		Devuelve uan lista con los puntos de montaje de las particiones ntfs y vfat montadas"""
		try:
			f = open("/etc/mtab",'r')
		except:
			self.error('No se puede determinar las unidades montadas')
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
		
	def search_win_users(self):
		"""Busca posibles usuarios de Windows en la particiones detectadas"""
		#directorios que no se deben tener en cuenta
		excluir = ('All Users', 'Default User', 'LocalService', 'NetworkService') 
		paths = []
		for part in self.win_parts:
			if os.path.exists(os.path.join(part, "Documents and Settings")):
				documents = os.listdir(os.path.join(part, "Documents and Settings"))
				for d in documents:
					ruta = os.path.join(part, 'Documents and Settings', d)
					if (not d in excluir) and (os.path.exists(os.path.join(ruta, 'NTUSER.DAT')) or os.path.exists(os.path.join(ruta, 'ntuser.dat'))):
						paths.append(ruta)
		return paths
	
	def get_win_users(self):
		"""Devuelve una lista con la ruta a las carpetas de los usuarios de Windows"""
		return self.win_users
	
	def get_windows(self):
		"""Devuelve las particiones que contienen un Sistema Operativo Windows instalado"""
		wins = []
		for e in self.win_parts:
			if os.path.exists(os.path.join(e, 'WINDOWS')):
				wins.append(e)
		return wins
	
#end class pc
	
class MSwindow (registry.winregistry):
	"""Clase para el manejo de la informacion del Sistema Operativo Windows"""
	def __init__(self, path):
		"""Constructor de la clase.
		Recibe la partición donde existe un Windows instalado"""
		registry.winregistry.__init__(self, path)
		self.path = path
	
#end class MSwindow
	
class user(registry.winregistry):
	"""Clase para los usuarios de Windows"""
	
	def __init__(self, path):
		"""Constructor de la clase.
		Recibe la ruta a la carpeta del usuario de Windows"""
		registry.winregistry.__init__(self, path)
		self.folders = self.get_user_folders()
		if not self.folders:
			self.error("Couldn't get user folders. Some Windows partition is not accessible")
		else:
			self.path = path
			self.name = os.path.split(path)[1]
			self.gaim = messenger.gaim()
			self.firefox = webbrowser.firefox(self.folders)
			self.thunderbird = mail.thunderbird()
			self.info = None
		if self.errors: print self.errors
		
	def get_path(self):
		"""Devuelve la carpeta del usuario"""
		return self.path()
	
	def get_personal_folder(self):
		"""Devuelve las carpetas personales y de configuración del usuario"""
		return self.folders['Personal']
	
	def get_name(self):
		"""Devuelve el nombre del usuario de Windows"""
		return self.name
	
	def get_messenger_accounts(self):
		"""Devuelve las cuentas de mensajería de Windows Live Messenger"""
		accounts = []
		live_msn = folder(os.path.join(self.folders['Local AppData'].path, 'Microsoft', 'Messenger'), False)
		if live_msn.path and os.path.exists(live_msn.path):
			for m in os.listdir(live_msn.path):
				if m.find('@') != -1:
					accounts.append(m)
		return accounts

	def config_aMule(self):
		"""Configura el programa de P2P aMule con la configuración del programa eMule en Windows"""
		amule = folder(os.path.join(os.path.expanduser('~'), '.aMule'))
		emule = folder(os.path.join(self.folders['eMule'].path,'config'), False)
		if amule and emule:
			# copy edonkey config files
			emule.copy(amule.path, extension = ['.ini','.dat','.met'])
			# copy edonkey temporary files
			conf = os.path.join(amule.path, 'amule.conf')
			bak = backup(conf)
			try:
				emupref = open(os.path.join(amule.path,'preferences.ini'),'r')
				amuconf = open(conf, 'w')
				for l in emupref.readlines():
					if l.find("IncomingDir") != -1:
						l = "IncomingDir=%s\n" % os.path.join(amule.path,'Incoming')
					if l.find("TempDir") != -1:
						l = "TempDir=%s\n" % os.path.join(amule.path,'Temp')
					amuconf.write(l)
				amuconf.close()
				emupref.close()
			except:
				amuconf.close()
				emupref.close()
				if bak:
					restore_backup(bak)
				self.error ('Fallo al configurar aMule')
		else:
			self.error("aMule no ha sido configurado")
			
	def import_eMule_files(self):
		"""Importa los archivos descargados con eMule"""
		amule = folder(os.path.join(os.path.expanduser('~'), '.aMule'))
		amule_inc = folder(os.path.join(amule.path, 'Incoming'))
		emule_inc= self.get_incoming_path()
		if amule and emule_inc and amule_inc:
			# copy edonkey downloaded files
			emule_inc.copy(amule_inc.path)
		else:
			self.error("Archivos Descargados de eMule no copiados")
			
	def import_eMule_temp(self):
		"""Importa los archivos temporales con eMule"""
		amule = folder(os.path.join(os.path.expanduser('~'), '.aMule'))
		amule_temp = folder(os.path.join(amule.path, 'Temp'))
		emule_temp = self.get_temp_path()
		if amule and emule_temp and amule_temp:
			# copy edonkey temporary files
			emule_temp.copy(amule_temp.path, extension = ['.part','.met',',part.met'])
		else:
			self.error("Archivos Temporales de eMule no copiados")
			
	def get_incoming_path(self):
		"""Devuelve la ruta del directorio de descargas"""
		try:
			a = open (os.path.join(self.folders['eMule'].path,'config','preferences.ini'), 'r')
			l = " "
			inc = os.path.join(self.folders['eMule'].path,'Incoming')
			for l in a.readlines():
				l = l.replace('\r\n','')
				if l.find("IncomingDir") != -1:
					inc = l.split('=')[1]
					inc = inc.replace('\\','/')
					inc = self.mount_points[inc[:2]] + inc[2:]
					a.close()
					return folder(inc)
		except:
			self.error("Failed to detect eMule paths")
			a.close()
			
	def get_temp_path(self):
		"""Devuelve la ruta del directorio de temporales"""
		try:
			a = open (os.path.join(self.folders['eMule'].path,'config','preferences.ini'), 'r')
			l = " "
			temp = os.path.join(self.folders['eMule'].path,'Temp')
			for l in a.readlines():
				l = l.replace('\r\n','')
				if l.find("TempDir") != -1:
					temp = l.split('=')[1]
					temp = temp.replace('\\','/')
					temp = self.mount_points[temp[:2]] + temp[2:]
					a.close()
					return folder(temp)
		except:
			self.error("Failed to detect eMule paths")
			a.close()
			
	def get_info(self):
		"""Devuelve la información de archivos y programas del usuario"""
		if not self.info:
			self.info = "Información del Usuario de Windows: %s\n" % self.name
			self.info = self.info + "- Archivos:\n"
			self.info = self.info + "\tMis documentos: %s\n" % self.folders['Personal'].get_info()
			self.info = self.info + "\tEscritorio: %s\n" % self.folders['Desktop'].get_info()
			#detect eMule installed
			if 'eMule' in self.folders.keys():
				emule_inc = self.get_incoming_path()
				emule_temp = self.get_temp_path()
				if emule_inc:
					self.info  = self.info + "\tDescargados de eMule: %s\n" % emule_inc.get_info()
				if emule_temp:
					self.info  = self.info + "\tTemporales de eMule: %s\n" % emule_temp.get_info()
			self.info = self.info + "- Programas:\n"
			#get installed software
			if os.path.exists(os.path.join(self.folders['AppData'].path,'Opera','Opera','profile','opera6.adr')):
				self.info = self.info + "\tNavegador Opera instalado\n"
			if os.path.exists(os.path.join(self.folders['AppData'].path,'Mozilla','Firefox')):
				self.info = self.info + "\tNavegador Mozilla-Firefox instalado\n"
			if self.thunderbird.is_installed_on_Windows(self.folders['AppData']):
				self.info = self.info + "\tCliente de correo Mozilla-Thundebird instalado\n"	
			self.info = self.info + "- Configuraciones:\n"
			#get IM accounts
			msn = self.get_MSN_account()
			yahoo = self.get_YAHOO_account()
			gtalk = self.get_GTALK_account()
			aol = self.get_AOL_accounts()
			if msn:
				self.info = self.info + "\tCuenta .NET Passport: %s\n" % msn
			if yahoo:
				self.info = self.info + "\tCuenta de Yahoo: %s\n" % yahoo
			if gtalk:
				self.info = self.info + "\tCuenta de Gtalk: %s\n" % gtalk
			for a in aol:
				self.info = self.info + "\tCuenta de AOL: %s\n" % a
			#get mail accounts settings
			naccountsOutlook, naccountsOutlookExpress = self.num_OUTLOOK_accounts()
			if naccountsOutlook:
				self.info = self.info + "\t%d cuentas de correo configuradas en Oulook\n" % naccountsOutlook
			if naccountsOutlookExpress:
				self.info = self.info + "\t%d cuentas de correo configuradas en OulookExpress\n" % naccountsOutlookExpress
		return self.info
			
	def all_errors(self):
		"""Recopila los errores producidos en tiempo de ejecuccion"""
		self.errors = self.errors + self.gaim.errors + self.firefox.errors + self.thunderbird.errors
		return self.errors
		
#end class user

if __name__ == "__main__":
	computer = pc()
	for u in computer.get_win_users():
		usuario = user(u)
		try:
			docs = usuario.get_personal_folder()
			info = docs.get_info()
			print 'Encontrado usuario: %s [Mis documentos: %s]' % (usuario.get_name(), info)
		except:
			pass
		usuario.clean()
		
		
