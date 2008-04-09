#!/usr/bin/python
# -*- coding: utf-8 -*-
# Este módulo aporta rutinas para configurar los clientes de mensajería instantánea:
#  - Gaim
#  - Kopete
#  - aMSN
# Tanto en las clases Gaim como Kopete se permite la configuración de multiples protocolos como:
#  - MSN
#  - Yahoo!
#  - Gtalk (basado en Jabber)

import os, re
from folder import *

class gaim:
	"""Clase para el programa Gaim"""
	
	def __init__(self):
		"""Constructor de la clase"""
		self.errors = []
		self.config_file = os.path.join(os.path.expanduser('~'), '.gaim', 'accounts.xml')
		if not os.path.exists(self.config_file):
			folder(os.path.join(os.path.expanduser("~"),'.gaim'))
			try:
				f = open (self.config_file, "w")
				f.write('<?xml version=\'1.0\' encoding=\'UTF-8\' ?>\n')
				f.write('<account version=\'1.0\'>\n')
				f.write('</account>')
				f.close()
			except:
				self.error('Failed to create %s' % config_file)
				return 0
	
	def error(self,e):
		"""Almacena los errores en tiempo de ejecución"""
		self.errors.append(e)
		
	def config_msn(self, cuenta):
		"""Configura la cuenta de MSN"""
		bak = backup(self.config_file)
		if bak:
			try:
				o = open (bak, "r")
				f = open (self.config_file, "w")
				for linea in o.readlines():
					f.write(linea)
					if re.search('account version',linea): # insert after that line (only once)
						f.write(' <account>\n')
						f.write('  <protocol>prpl-msn</protocol>\n')
						f.write('  <name>%s</name>\n' % cuenta)
						f.write('  <settings>\n')
						f.write('  <setting name=\'check-mail\' type=\'bool\'>1</setting>\n')
						f.write('  <setting name=\'server\' type=\'string\'>messenger.hotmail.com</setting>\n')
						f.write('  <setting name=\'http_method\' type=\'bool\'>0</setting>\n')
						f.write('  <setting name=\'port\' type=\'int\'>1863</setting>\n')
						f.write('  </settings>\n')
						f.write('  <settings ui=\'gtk-gaim\'>\n')
						f.write('  <setting name=\'auto-login\' type=\'bool\'>0</setting>\n')
						f.write('  </settings>\n')
						f.write(' </account>\n')
					# write de rest of de original config
				o.close()
				f.close()
			except:
				self.error('Failed to modify %s' % self.config_file)
				restore_backup(bak)
				o.close()
				f.close()
		else:
			self.error("Config Gaim failed")
	
	def config_yahoo(self, cuenta):
		"""Configura la cuenta de Yahoo!"""
		bak = backup(self.config_file)
		if bak:
			try:
				o = open (bak, "r")
				f = open (self.config_file, "w")
				for linea in o.readlines():
					f.write(linea)
					if re.search('account version',linea): # insert after that line (only once)
						f.write(' <account>\n')
						f.write('  <protocol>prpl-yahoo</protocol>\n')
						f.write('  <name>%s</name>\n' % cuenta)
						f.write('  <settings>\n')
						f.write('  <setting name=\'check-mail\' type=\'bool\'>1</setting>\n')
						f.write('  <setting name=\'server\' type=\'string\'>scs.msg.yahoo.com</setting>\n')
						f.write('  <setting name=\'yahoojp\' type=\'bool\'>0</setting>\n')
						f.write('  <setting name=\'port\' type=\'int\'>5050</setting>\n')
						f.write('  <setting name=\'serverjp\' type=\'string\'>cs.yahoo.co.jp</setting>\n')
						f.write('  <setting name=\'port\' type=\'int\'>5050</setting>\n')
						f.write('  <setting name=\'xferjp_host\' type=\'string\'>filetransfer.msg.yahoo.co.jp</setting>\n')
						f.write('  <setting name=\'xfer_host\' type=\'string\'>filetransfer.msg.yahoo.com</setting>\n')
						f.write('  <setting name=\'xfer_port\' type=\'int\'>80</setting>\n')
						f.write('  </settings>\n')
						f.write('  <settings ui=\'gtk-gaim\'>\n')
						f.write('  <setting name=\'auto-login\' type=\'bool\'>0</setting>\n')
						f.write('  </settings>\n')
						f.write(' </account>\n')
					# write de rest of de original config
				o.close()
				f.close()
			except:
				self.error('Failed to modify %s' % self.config_file)
				restore_backup(bak)
				o.close()
				f.close()
		else:
			self.error("Config Gaim failed")
				
	def config_gtalk (self, cuenta):
		"""Configura la cuenta de Gtalk"""
		bak = backup(self.config_file)
		if bak:
			try:
				o = open (bak, "r")
				f = open (self.config_file, "w")
				for linea in o.readlines():
					f.write(linea)
					if re.search('account version',linea): # insert after that line (only once)
						f.write(' <account>\n')
						f.write('  <protocol>prpl-jabber</protocol>\n')
						f.write('  <name>%s/Home</name>\n' % cuenta)
						f.write('  <settings>\n')
						f.write('  <setting name=\'require_tls\' type=\'bool\'>0</setting>\n')
						f.write('  <setting name=\'use_tls\' type=\'bool\'>1</setting>\n')
						f.write('  <setting name=\'connect_server\' type=\'string\'>talk.google.com</setting>\n')
						f.write('  <setting name=\'old_ssl\' type=\'bool\'>1</setting>\n')
						f.write(' <setting name=\'port\' type=\'int\'>5223</setting>\n')
						f.write('  </settings>\n')
						f.write('  <settings ui=\'gtk-gaim\'>\n')
						f.write('  <setting name=\'auto-login\' type=\'bool\'>0</setting>\n')
						f.write('  </settings>\n')
						f.write(' </account>\n')
					# write de rest of de original config
				o.close()
				f.close()
			except:
				self.error('Failed to modify %s' % self.config_file)
				restore_backup(bak)
				o.close()
				f.close()
		else:
			self.error("Config Gaim failed")
				
# end class gaim
	
############################################################################


class kopete:
	"""Clase para el programa Kopete"""
	
	def __init__(self):
		"""Constructor de la clase"""
		self.errors = []
		self.config_file = os.path.join(os.path.expanduser('~'), '.kde', 'share', 'config', 'kopeterc')
		folder(os.path.join(os.path.expanduser('~'), '.kde', 'share', 'config'))

	def error(self, e):
		"""Almacena los errores en tiempo de ejecución"""
		self.errors.append(e)
		
	def config_msn(self, cuenta):
		"""Configura la cuenta de MSN"""
		bak = backup(self.config_file)
		try:
			f = open (self.config_file, "a")
			#insert at the bottom, the program automatically will reorder the file
			f.write('\n[Account_MSNProtocol_%s]\n' % cuenta)
			f.write('AccountId=%s\n' % cuenta)
			f.write('Protocol=MSNProtocol\n')
			f.write('serverName=messenger.hotmail.com\n')
			f.write('serverPort=1863\n')
			f.write('useHttpMethod=false\n')
			f.close()
		except:
			f.close()
			self.error("Failed to modify %s" % self.config_file)	
			restore_backup(bak)
			
	def config_yahoo(self, cuenta):
		"""Configura la cuenta de Yahoo!"""
		bak = backup(self.config_file)
		try:
			f = open (self.config_file, "a")
			#insert at the bottom, the program automatically will reorder the file
			f.write('\n[Account_YahooProtocol_%s]\n' % cuenta)
			f.write('AccountId=%s\n' % cuenta)
			f.write('Protocol=YahooProtocol\n')
			f.write('Server=scs.msg.yahoo.com\n')
			f.write('Port=5050\n')
			f.close()
		except:
			f.close()
			self.error("Failed to modify %s" % self.config_file)	
			restore_backup(bak)
			
	def config_gtalk(self, cuenta):
		"""Configura la cuenta de Gtalk"""
		bak = backup(self.config_file)
		try:
			f = open (self.config_file, "a")
			#insert at the bottom, the program automatically will reorder the file
			f.write('\n[Account_YahooProtocol_%s]\n' % cuenta)
			f.write('\n[Account_JabberProtocol_%s]\n' % cuenta)
			f.write('AccountId=%s\n' % cuenta)
			f.write('AllowPlainTextPassword=true\n')
			f.write('CustomServer=true\n')
			f.write('Server=talk.google.com\n')
			f.write('Port=5223\n')
			f.write('HideSystemInfo=false\n')
			f.write('Protocol=JabberProtocol\n')
			f.write('UseSSL=true\n')
			f.close()
		except:
			f.close()
			self.error("Failed to modify %s" % self.config_file)	
			restore_backup(bak)

# end class kopete
	
############################################################################

def msn2amsn(cuenta):
	"""Configura la cuenta de MSN que se le pasa en aMSN"""
	destino = folder(os.path.join(os.path.expanduser("~"),'.amsn'))
	if destino:
		if os.path.exists(os.path.join(destino.path, 'profiles')):
			bak = backup(os.path.join(destino.path, 'profiles'))
			try:
				f = open (os.path.join(destino.path, 'profiles'), "a")
				#insert at the bottom, the program automatically will reorder the file
				f.write(cuenta + ' 0\n')
				f.close()
			except:
				f.close()
				error("Config aMSN failed")
				restore_backup(bak)
		else:
			try:
				f = open (os.path.join(destino.path, 'profiles'), "w")
				f.write('amsn_profiles_version 1\n')
				f.write(cuenta + ' 0\n')
				f.close()
			except:
				f.close()
				error("Config aMSN failed")


if __name__ == "__main__":
	g = gaim()
	k = kopete()
	g.config_msn('test@hotmail.com')
	g.config_yahoo('test')
	g.config_gtalk('test@gmail.com')
	k.config_msn('test@hotmail.com')
	k.config_yahoo('test')
	k.config_gtalk('test@gmail.com')
	msn2amsn('test@hotmail.com')
	print g.errors
	print k.errors
