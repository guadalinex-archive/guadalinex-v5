#!/usr/bin/python
# -*- coding: utf-8 -*-

import os, re, random, time, registry, shutil
from folder import *

__DIR_PST2MBX__="/usr/bin"
__DIR_DBX2MBX__="/usr/bin"


class mailconfig:
	"""Clase para las configuraciones de correo.
	Visit http://msdn2.microsoft.com/en-us/library/ms715237.aspx for more information about the values"""
	
	def __init__(self, config):
		"""Constructor de la clase.
		Recibe la configuración obtenida del registro"""
        	self.c = config
		
	def get_type(self):
		"""Devuelve el tipo de cuenta de correo"""
		if 'IMAP Server' in self.c.keys():
			t = "imap"
		elif 'POP3 Server' in self.c.keys():
			t = "pop3"
		elif 'HTTPMail Server' in self.c.keys():
			t = "HTTPMail"
		else:
			t = "Unknown"
		return t
	
	def get_user_name(self):
		"""Devuelve el nombre del usuario"""
		if self.get_type() == "imap":
			r = self.c['IMAP User Name']
		elif self.get_type() == "pop3":
			r = self.c['POP3 User Name']
		elif self.get_type() == "HTTPmail":
			r = self.c['HTTPMail User Name']
		else:
			r = None
		return r
	
	def get_account_name(self):
		"""Devuelve el nombre de la cuenta de correo"""
		return self.c['Account Name']
		
	def get_connection_type(self):
		"""Devuelve el tipo de conexión con el servidor"""
		return self.c['Connection Type']
	
	def get_server(self):
		"""Devuelve el servidor de correo entrante"""
		if self.get_type() == "imap":
			r = self.c['IMAP Server']
		elif self.get_type() == "pop3":
			r = self.c['POP3 Server']
		elif self.get_type() == "HTTPMail":
			r = self.c['HTTPMail Server']
		else:
			r = None
		return r
		
	def use_SSL(self):
		"""Devuelve si la conexión es segura"""
		if self.get_type() == "imap" and 'IMAP Secure Connection' in self.c.keys():
			r = hex2dec(self.c['IMAP Secure Connection'])
		elif self.get_type() == "pop3" and 'POP3 Secure Connection' in self.c.keys():
			r = hex2dec(self.c['POP3 Secure Connection'])
		else:
			r = None
		return r
	
	def get_port(self):
		"""Devuelve el puerto del servidor de correo entrante"""
		if self.get_type() == "imap" and ('IMAP Port' in self.c.keys()):
			r = hex2dec(self.c['IMAP Port'])
		elif self.get_type() == "pop3" and ('POP3 Port' in self.c.keys()):
			r = hex2dec(self.c['POP3 Port'])
		else:
			r = None
		return r
		
	def get_timeout(self):
		"""Devuelve el tiempo de espera"""
		if self.get_type() == "imap":
			r = self.c['IMAP Timeout']
		elif self.get_type() == "pop3":
			r = self.c['POP3 Timeout']
		else:
			r = None
		return r
	
	def remove_expired(self):
		"""Devuelve si se eliminan los correos con el paso del tiempo"""
		if 'Remove When Expired' in self.c.keys():
			return hex2dec(self.c['Remove When Expired']) and 'true' or 'false'
		
	def remove_deleted(self):
		"""Devuelve si se eliminan los correos del servidor al ser borrados"""
		if 'Remove When Deleted' in self.c.keys():
			return hex2dec(self.c['Remove When Deleted']) and 'true' or 'false'
		
	def get_SMTP_display_name(self):
		"""Devuelve el nombre del servidor de correo saliente"""
		return self.c['SMTP Display Name']

	def get_SMTP_port(self):
		"""Devuelve el puerto del servidor de correo saliente"""
		if 'SMTP Port' in self.c.keys():
			return hex2dec(self.c['SMTP Port'])
		
	def get_SMTP_server(self):
		"""Devuelve el servidor de correo saliente"""
		return self.c['SMTP Server']
	
	def get_SMTP_email_address(self):
		"""Devuelve la direccion de correo saliente"""
		return self.c['SMTP Email Address']
		
	def get_SMTP_timeout(self):
		"""Devuelve el tiempo de expera del servidor de correo saliente"""
		return hex2dec(self.c['SMTP Timeout'])
				
	def use_SMTP_SSL(self):
		"""Devuelve si la conexión con el servidor de correo saliente es segura"""
		if 'SMTP Secure Connection' in self.c.keys():
			return hex2dec(self.c['SMTP Secure Connection'])
		
	def leave_mail(self):
		"""Devuelve si los mensajes deben conservarse en el servidor"""
		r = 'false'
		if 'Leave Mail On Server' in self.c.keys():
			r = hex2dec(self.c['Leave Mail On Server']) and 'true' or 'false'
		return r
		
	def get_mailbox(self, localdata):
		"""Importa y convierte los correos almacenados (OBSOLETO)"""
		pstpath = os.path.join(localdata,'Microsoft','Outlook')
		print pstpath
		if os.path.exists(pstpath):
			for e in os.listdir(pstpath):
				if e.find(self.get_account_name()) != -1:
					orig = os.path.join(pstpath, e)
					readpst = os.path.join(__DIR_PST2MBX__,'readpst')
					temp = folder(os.path.join('/tmp',e))
					dest = folder(os.path.join(os.path.expanduser('~'),'.evolution','mail','local'))
					com = '%s -w %s -o %s' % (readpst, orig.replace(' ',"\ "), temp.path.replace(' ',"\ "))
					try:
						os.system (com)
					except:
						error("Failed to convert mailboxes")
					else:
						for mbox in os.listdir(temp.path):
							try:
								mbox_size = os.stat(os.path.join(temp.path,mbox))[6]
								if mbox_size:
									shutil.copy2(os.path.join(temp.path,mbox), os.path.join(dest.path, self.get_account_name() + ' - ' + mbox))
							except:
								error("Failed to import mailboxes")
								shutil.rmtree(temp.path)
					
# end class mail

############################################################################

def hex2dec(hexa):
	"""Convierte valores hexadecimales del registro en valores enteros"""
	hexa = hexa.replace('dword:','0x')
	return int(hexa, 16)


class thunderbird:
	"""Clase para el gestro de correo Thunderbird"""
	
	def __init__(self):
		"""Constructor de la clase"""
		thunderbird = folder(os.path.join(os.path.expanduser('~'),'.mozilla-thunderbird'))
		self.profile = self.get_thunderbird_profile(thunderbird.path)
		self.errors = []
		if not self.profile and thunderbird.path:
			try:
				prof = open(os.path.join(thunderbird.path,'profiles.ini'), 'w')
				prof.write("[General]\n")
				prof.write("StartWithLastProfile=1\n\n")
				prof.write("[Profile0]\n")
				prof.write("Name=migrated\n")
				prof.write("IsRelative=1\n")
				prof.write("Path=m1gra73d.default\n")
				prof.close()
				self.profile = thunderbird.create_subfolder("m1gra73d.default")
			except:
				self.error("Unable to create Mozilla Thunderbird profile")
				return 0
		self.config_file  = os.path.join(self.get_thunderbird_profile(thunderbird.path), 'prefs.js')
		
	def error (self, e):
		"""Almacena los errores en tiempo de ejecución"""
		self.errors.append(e)
		
	def get_thunderbird_profile(self, thunderbird = '~/.mozilla-thunderbird'):
		"""Devuelve el perfil actual de Thunderbird. En caso de no existir crea uno nuevo"""
		profiles_ini = os.path.join(thunderbird,'profiles.ini')
		if os.path.exists(profiles_ini):
			try:
				prof = open(profiles_ini, 'r')
			except:
				self.error ('Unable to read Thunderbird profiles')
			else:
				relative = 0
				for e in prof.read().split('\n'):
					if re.search("IsRelative=1",e):
						relative = 1
					if re.search("Path",e):
						profile = e.split('=')[1]
				prof.close()
				if relative:
					path_profile = os.path.join(thunderbird, profile)
				else:
					path_profile = profile
				if path_profile[-1]=='\r':
					return path_profile[:-1]
				else:
					return path_profile

	def config_account(self, account):
		"""Configura la cuenta leida del registro"""
		n = str(random.randint(100,999))
		#check if the mail config is valid
		a = mailconfig(account)
		server_type = a.get_type()
		if server_type == "HTTPMail" or server_type == "Unknown":
			warning("Acoount type %s won't be added" % server_type)
			return 0
		elif os.path.exists(self.config_file):
			bak = backup (self.config_file)
			try:
				p = open(self.config_file, 'w')
				b = open(bak, 'r')
			except:
				p.close()
				b.close()
				self.error('Unable to modify %s' % self.config_file)
				return 0
			else:
				added_account, added_smtpserver = False, False
				for l in b.readlines():
					if re.search('mail.accountmanager.accounts',l): #add the new account
						l = l[:-5] + ',account' + n + l[-5:]
						added_account = True
					if re.search('mail.smtpservers', l): #add the new smtp
						l = l[:-4] + ',smtp' + n + l[-4:]
						added_smtpserver = True
					p.write(l)
				if not added_account:
					p.write("user_pref(\"mail.accountmanager.accounts\", \"account%s\");\n" % n)
				if not added_smtpserver:
					p.write("user_pref(\"mail.smtpservers\", \"smtp%s\");\n" % n)
				b.close()
		else:
			try:
				p = open(self.config_file, 'w')
			except:
				self.error('Unable to create %s' % self.config_file)
				return 0
			else:
				p.write("# Mozilla User Preferences\n\n")
				p.write("user_pref(\"mail.accountmanager.accounts\", \"account%s\");\n" % n)
				p.write("user_pref(\"mail.smtpservers\", \"smtp%s\");\n" % n)
				p.write("user_pref(\"mail.root.none\", \"%s/Mail\");\n" % self.profile)
				p.write("user_pref(\"mail.root.none-rel\", \"[ProfD]Mail\");\n")
				if server_type == "imap":# only for IMAP accounts
					p.write("user_pref(\"mail.root.imap\", \"%s/ImapMail\");\n" % self.profile)
					p.write("user_pref(\"mail.root.imap-rel\", \"[ProfD] ImapMail\");\n")
					#p.write("user_pref(\"mail.server.server%s.directory\", \"%s/ImapMail/%s\");\n"% (n, self.profile, a.get_server()))
				elif server_type == "pop3": # only for POP3 accounts
					p.write("user_pref(\"mail.root.pop3\", \"%s/Mail\");\n" % self.profile)
					p.write("user_pref(\"mail.root.pop3-rel\", \"[ProfD]Mail\");\n")
					#p.write("user_pref(\"mail.server.server%s.leave_on_server\", %s);\n" % (n, a.leave_mail()))
					#p.write("user_pref(\"mail.server.server%s.directory\", \"%s/Mail/%s\");\n" % (n, self.profile, a.get_server()))
					#p.write ("user_pref(\"mail.server.server%s.delete_by_age_from_server\", %s);\n" % (n, a.remove_deleted()))
					#p.write("user_pref(\"mail.server.server%s.delete_mail_left_on_server\", %s);\n" % (n, a.remove_expired()))
		# for both types
		p.write("user_pref(\"mail.account.account%s.identities\", \"id%s\");\n" % (n, n))
		p.write("user_pref(\"mail.account.account%s.server\", \"server%s\");\n" % (n, n))
		# identity configuration
		p.write("user_pref(\"mail.identity.id%s.fullName\", \"%s\");\n" % (n, a.get_SMTP_display_name()))
		p.write("user_pref(\"mail.identity.id%s.useremail\", \"%s\");\n" % (n, a.get_SMTP_email_address()))
		p.write("user_pref(\"mail.identity.id%s.smtpServer\", \"smtp%s\");\n" % (n, n))
		p.write("user_pref(\"mail.identity.id%s.valid\", true);\n" % n )
		# server configuration
		p.write("user_pref(\"mail.server.server%s.login_at_startup\", true);\n" % n )
		p.write("user_pref(\"mail.server.server%s.name\", \"%s\");\n" % (n, a.get_account_name()))
		p.write("user_pref(\"mail.server.server%s.hostname\", \"%s\");\n"% (n, a.get_server()))
		p.write("user_pref(\"mail.server.server%s.type\", \"%s\");\n" % (n, server_type))
		p.write("user_pref(\"mail.server.server%s.userName\", \"%s\");\n" % (n, a.get_user_name()))
		if a.use_SSL():
			p.write("user_pref(\"mail.server.server%s.socketType\", 3);\n"% n)
			p.write("user_pref(\"mail.server.server%s.port\", %d);\n" % (n, a.get_port()))
		# SMTP configuration
		p.write("user_pref(\"mail.smtpserver.smtp%s.hostname\", \"%s\");\n" % (n, a.get_SMTP_server()))
		p.write("user_pref(\"mail.smtpserver.smtp%s.username\", \"%s\");\n" % (n, a.get_user_name()))
		if a.use_SMTP_SSL():
			p.write("user_pref(\"mail.smtpserver.smtp%s.try_ssl\", 3);\n" % n)
			p.write("user_pref(\"mail.smtpserver.smtp%s.port\", %d);\n" % (n, a.get_SMTP_port()))
		p.close()
	
	def import_windows_settings(self, AppData):
		"""Importa la configuración de Thunderbird en Windows. SOBREESCRIBE LA INFORMACIÓN EXISTENTE"""
		winprofile = folder(self.get_thunderbird_profile(os.path.join(AppData, 'Thunderbird')))
		if winprofile:
			# copy all files from the winprofile
			winprofile.copy(self.profile)
			# modify prefs.js
			try:
				wp = open (os.path.join(winprofile.path, 'prefs.js'), 'r')
				p = open (self.config_file, 'w')
				for l in wp.readlines():
					if l.find(':\\') == -1:
						p.write(l)
			except:
				self.error("Failed to copy Thunderbird profile")
			wp.close()
			p.close()
		return 0
					
	def is_installed_on_Windows(self, AppData):
		"""Devuelve si Firefox está instalado en Windows"""
		return self.get_thunderbird_profile(os.path.join(AppData.path, 'Thunderbird'))
	
	def generate_impab(self, l):
		try:
			backup(os.path.join(self.profile,'impab.mab'))
			f = open(os.path.join(self.profile,'impab.mab'),'w')
		except:
			self.error( "Fallo al crear contactos de Thunderbird")
		else:
			f.write("// <!-- <mdb:mork:z v=\"1.4\"/> -->\n< <(a=c)> // (f=iso-8859-1)\n  (B8=Custom3)(B9=Custom4)(BA=Notes)(BB=LastModifiedDate)(BC=RecordKey)\n  (BD=AddrCharSet)(BE=LastRecordKey)(BF=ns:addrbk:db:table:kind:pab)\n  (C0=ListName)(C1=ListNickName)(C2=ListDescription)\n  (C3=ListTotalAddresses)(C4=LowercaseListName)\n  (C5=ns:addrbk:db:table:kind:deleted)\n  (80=ns:addrbk:db:row:scope:card:all)\n  (81=ns:addrbk:db:row:scope:list:all)\n  (82=ns:addrbk:db:row:scope:data:all)(83=FirstName)(84=LastName)\n  (85=PhoneticFirstName)(86=PhoneticLastName)(87=DisplayName)\n  (88=NickName)(89=PrimaryEmail)(8A=LowercasePrimaryEmail)\n  (8B=SecondEmail)(8C=DefaultEmail)(8D=CardType)(8E=PreferMailFormat)\n  (8F=PopularityIndex)(90=WorkPhone)(91=HomePhone)(92=FaxNumber)\n  (93=PagerNumber)(94=CellularNumber)(95=WorkPhoneType)(96=HomePhoneType)\n  (97=FaxNumberType)(98=PagerNumberType)(99=CellularNumberType)\n  (9A=HomeAddress)(9B=HomeAddress2)(9C=HomeCity)(9D=HomeState)\n  (9E=HomeZipCode)(9F=HomeCountry)(A0=WorkAddress)(A1=WorkAddress2)\n  (A2=WorkCity)(A3=WorkState)(A4=WorkZipCode)(A5=WorkCountry)\n  (A6=JobTitle)(A7=Department)(A8=Company)(A9=_AimScreenName)\n  (AA=AnniversaryYear)(AB=AnniversaryMonth)(AC=AnniversaryDay)\n  (AD=SpouseName)(AE=FamilyName)(AF=DefaultAddress)(B0=Category)\n  (B1=WebPage1)(B2=WebPage2)(B3=BirthYear)(B4=BirthMonth)(B5=BirthDay)\n  (B6=Custom1)(B7=Custom2)>\n\n")
	
			f.write("<(80=0)>\n{1:^80 {(k^BF:c)(s=9)}\n  [1:^82(^BE=0)]}\n\n")
	
			f.write("@$${1{@\n\n<")
			i, j = 1, 1
			for a in l:
				f.write("(%d=%d)(%d=%s)"%(80 + 2*(len(l) - j + 1) , len(l) - j + 1 , 80 + i, a ))
				i += 2
				j += 1
			f.write(">\n{-1:^80 {(k^BF:c)(s=9)} \n  [1:^82(^BE=%d)]"%(j-1)) 
			i, j = 1, 1
			for a in l:
				f.write("\n  [-%d(^89^%d)(^8A^%d)(^BC=%d)]"%(j, 80+i, 80+i, j))
				i += 2
				j += 1
			f.write("}\n@$$}1}@")
			f.close()
			self.add_impab()
			return 1
			
	def add_impab(self):
		p = None
		if os.path.exists(self.config_file):
			bak = backup (self.config_file)
			try:
				p = open(self.config_file, 'a')
			except:
				p.close()
				self.error('Unable to modify %s' % self.config_file)
				return 0
				
		else:
			try:
				p = open(self.config_file, 'w')
			except:
				self.error('Unable to create %s' % self.config_file)
				return 0
			else:
				p.write("# Mozilla User Preferences\n\n")
				p.write("user_pref(\"mail.accountmanager.accounts\", \"\");\n")
				p.write("user_pref(\"mail.smtpservers\", \"\");\n")
				p.write("user_pref(\"mail.root.none\", \"%s/Mail\");\n"%self.profile)
				p.write("user_pref(\"mail.root.none-rel\", \"[ProfD]Mail\");\n")
		if p:
			p.write("user_pref(\"ldap_2.servers.contactosOutlook.description\", \"contactos_Outlook\");\n")
			p.write("user_pref(\"ldap_2.servers.contactosOutlook.dirType\", 2);\n")
			p.write("user_pref(\"ldap_2.servers.contactosOutlook.filename\", \"impab.mab\");\n")
			p.write("user_pref(\"ldap_2.servers.contactosOutlook.isOffline\", false);\n")
			p.write("user_pref(\"ldap_2.servers.contactosOutlook.protocolVersion\", \"2\");\n")
			p.write("user_pref(\"ldap_2.servers.contactosOutlook.replication.lastChangeNumber\", 0);\n")
			p.write("user_pref(\"ldap_2.servers.contactosOutlook.position\", 1);\n")
			p.close()

# end class thunderbird

############################################################################

	
def config_EVOLUTION(account, localdata = None):
	"""Configura la cuenta dada en Evolution"""
	#check if the mail config is valid
	a = mailconfig(account)
	if localdata:
		a.get_mailbox(localdata)
	ssl = a.use_SSL() and 'always' or 'never'
	server_type = a.get_type()
	if server_type == 'pop3':
		m = 'pop'
		draft = "mbox:" + os.path.expanduser('~') + "/.evolution/mail/local#Drafts"
		sent = "mbox:" + os.path.expanduser('~') + "/.evolution/mail/local#Sent"
		command = ''
	elif server_type == 'imap':
		m = 'imap'
		draft, sent = '', ''
		command = ";check_all;command=ssh%20-C%20-l%20%25u%20%25h%20exec%20/usr/sbin/imapd"
	elif server_type == "HTTPMail" or server_type == "Unknown":
		warning("Account type %s won't be added" % server_type)
		return 0
	# get previous accounts
	try:
		l = os.popen("gconftool --get /apps/evolution/mail/accounts")
		laccounts = str(l.read())
		if not laccounts:
			laccounts = "[]\n"
	except:
		error ("Evolution configuration is not readable")
	else:
		# generate new xml list
		e = "<?xml version=\"1.0\"?>\n" + \
		"<account name=\"%s\" uid=\"%s\" enabled=\"true\">" % (a.get_account_name(), str(time.time())) + \
		"<identity>" + \
		"<name>%s</name>" % a.get_SMTP_display_name() + \
		"<addr-spec>%s</addr-spec><signature uid=\"\"/>" % a.get_SMTP_email_address() + \
		"</identity>" + \
		"<source save-passwd=\"false\" keep-on-server=\"%s\" auto-check=\"true\" auto-check-timeout=\"10\">" % a.leave_mail() + \
		"<url>%s://%s@%s/%s;use_ssl=%s</url>" % (m, a.get_user_name(), a.get_server(), command, ssl) + \
		"</source>" + \
		"<transport save-passwd=\"false\">" + \
		"<url>smtp://%s;auth=PLAIN@%s/;use_ssl=%s</url>" % (a.get_user_name(), a.get_SMTP_server(), ssl) + \
		"</transport>" + \
		"<drafts-folder>%s</drafts-folder>" % draft + \
		"<sent-folder>%s</sent-folder>" % sent + \
		"<auto-cc always=\"false\"><recipients></recipients></auto-cc><auto-bcc always=\"false\"><recipients></recipients></auto-bcc><receipt-policy policy=\"never\"/>" + \
		"<pgp encrypt-to-self=\"false\" always-trust=\"false\" always-sign=\"false\" no-imip-sign=\"false\"/><smime sign-default=\"false\" encrypt-default=\"false\" encrypt-to-self=\"false\"/>" + \
		"</account>\n"
		# concatenate elemennt to the list
		if laccounts == "[]\n":
			#create list
			laccounts = "[" + e +"]"
		else:
			#add to list
			laccounts = laccounts[:-2] + ',' + e + "]"
		# set the modified list
		try:
			os.system("gconftool --set /apps/evolution/mail/accounts --type list --list-type string \"" + laccounts.replace("\"", "\\\"") +"\"")
			progress("Account %s added successfully" % a.get_account_name())
		except:
			error ("Evolution configuration is not writable")

class kmail:
	"""Clase para el gestor de correo Kmail integrado en Kontact"""
	
	def __init__(self):
		"""Constructor de la clase"""
		self.errors = []
		self.config_file = os.path.join(os.path.expanduser('~'),'.kde', 'share', 'config', 'kmailrc')
		if os.path.exists(self.config_file):
			try:
				b = open (self.config_file, 'r')
				for l in b.readlines():
					if re.search('accounts=',l): # increase account
						self.iaccount = int(l[9:]) + 1
					if re.search('transports=', l): # increase smtp
						self.itransport = int(l[11:]) + 1
				b.close()
			except:
				b.close()
				self.error("Unable to read %s" % self.config_file)
		else:
			self.iaccount, self.itransport = 0, 0
			try:
				b = open (self.config_file, 'r')
				b.write("[Composer]\n")
				b.write("default-transport=%s\n" % a.get_SMTP_display_name())
				b.write("\n[General]\n")
				b.write("accounts=1\n")
				b.write("checkmail-startup=true\n")
				b.write("first-start=false\n")
				b.write("transports=1\n")
				b.close()
			except:
				b.close()
				self.error("Unable to create %s" % self.config_file)
		self.identities = os.path.join(os.path.expanduser('~'), '.kde', 'share', 'config', 'emailidentities')
		self.maxid = 0
		if os.path.exists(self.identities):# configuration file already exists
			try:
				b = open(self.identities, 'r')
				for l in b.readlines():
					if re.search('Identity #',l): # increase account
						maxid = (maxid <= int(l[11:-2])) and int(l[11:-2]) + 1 or maxid
				b.close()
			except:
				b.close()
				self.error('Unable to access %s' % self.identities)

	
	def error(self, e):
		"""Almacena los errores en tiempo de ejecución"""
		self.errors.append(e)
	
	def config_account(self, account):
		"""Configura la cuenta dada en Kmail"""
		#check if the mail config is valid
		a = mailconfig(account)
		server_type = a.get_type()
		if server_type == "HTTPMail" or server_type == "Unknown":
			warning("Acoount type %s won't be added" % server_type)
			return 0
		self.itransport = self.itransport + 1
		self.iaccount = self.iaccount + 1
		bak = backup (self.config_file)
		if bak:
			try:
				p = open(self.config_file, 'w')
				# config account
				p.write("\n[Account %i]\n" % self.iaccount)
				p.write("Folder=inbox\n")
				p.write("Id=%s\n" % str(time.time()))
				p.write("Name=%s\n" % a.get_account_name())
				if server_type=='imap':
					p.write("auth=*\n")
					p.write("Type=%s\n" % server_type)
				if server_type=='pop3':
					p.write("auth=USER\n")
					p.write("Type=pop\n")
				p.write("host=%s\n" % a.get_server())
				p.write("leave-on-server=%s\n" % a.leave_mail())
				p.write("login=%s\n" % a.get_user_name())
				p.write("port=%s\n" % a.get_port())
				p.write("use-ssl=%s\n" % a.use_SSL())
				p.write("use-tls=false\n")
				#config transport (SMTP)
				p.write("\n[Transport %i]\n" % self.itransport)
				p.write("auth=false\n")
				p.write("authtype=PLAIN\n")
				p.write("encryption=SSL\n")
				p.write("host=%s\n" % a.get_SMTP_server())
				p.write("id=%s\n" % str(time.time()))
				p.write("name=%s\n" % a.get_SMTP_server())
				p.write("port=%s\n" % a.get_SMTP_port())
				p.write("type=smtp\n")
				p.write("user=\n")
				p.close()
				#config identity
				self.add_identity(account)
			except:
				p.close()
				self.itransport = self.itransport - 1
				self.iaccount = self.iaccount - 1
				self.error('Failed to modify %s' % self.config_file)
				return 0
		
	def add_identity(self, a):
		"""Añade la identidad"""
		bak = backup(self.identities)
		if bak:
			try:
				p = open(self.identities, 'a')
				p.write("\n[Identity #%i]\n" % self.maxid)
				p.write("Email Address=%s\n" % a.get_SMTP_email_address())
				p.write("Name=%s\n" % a.get_SMTP_display_name())
				p.write("Transport=%s\n" % a.get_SMTP_server())
				p.write("uoid=%s\n" % str(time.time()*100))
				p.close()
			except:
				p.close()
				self.error("Failed to add identity")
				restore_backup(bak)

# end class kmail

############################################################################

def convert_pst(pstfile):
	"""Convierte el archivo .pst que se le pasa y lo integra en Evolution"""
	if os.path.exists(pstfile):
		readpst = os.path.join(__DIR_PST2MBX__,'readpst')
		account = os.path.splitext(os.path.split(pstfile)[1])[0]
		temp = folder(os.path.join('/tmp',account))
		dest = folder(os.path.join(os.path.expanduser('~'),'.evolution','mail','local'))
		com = '%s -w %s -o %s' % (readpst, pstfile.replace(' ',"\ "), temp.path.replace(' ',"\ "))
		try:
			f = os.popen (com)
			if f.read().find("Outlook 2003 PST file") != -1:
				return "Encontrado PST de Outlook 2003"
		except:
			error("Failed to convert mailboxes")
		else:
			for mbox in os.listdir(temp.path):
				try:
					mbox_size = os.stat(os.path.join(temp.path,mbox))[6]
					if mbox_size:
						shutil.copy2(os.path.join(temp.path,mbox), os.path.join(dest.path, account + ' - ' + mbox))
				except:
					error("Failed to import mailboxes")
					shutil.rmtree(temp.path)
					
					
def convert_dbx(dbxfile):
	"""Convierte el archivo .dbx que se le pasa y lo integra en Evolution"""
	if os.path.exists(dbxfile):
		readdbx = os.path.join(__DIR_DBX2MBX__,'readoe')
		dest = folder(os.path.join(os.path.expanduser('~'),'.evolution','mail','local'))
		com = '%s -i %s -o %s' % (readdbx, os.path.split(dbxfile)[0].replace(' ',"\ "),dest.path)
		try:
			os.system (com)
		except:
			error("Failed to convert mailboxes")
			
def read_evolution_addressbook(path='~/.evolution/addressbook/local/system/addressbook.db'):
	"""Lee los contactos alamcenados en Evolution"""
	print 'Cargando modulo de base de datos Berkeley'
	import bsddb
	print 'Abriendo addressbook de Evolution'
	db = bsddb.hashopen(path,'w')
	#print 'Insertando direcciones'
	#i = 1
	#for m in ab:
		#print m
		#randomid = 'pas-id-' + str(random.random())[2:]
		#db[randomid+'\x00'] = 'BEGIN:VCARD\r\nVERSION:3.0\r\nEMAIL;TYPE=OTHER;X-EVOLUTION-UI-SLOT=1:'+ m +'\r\nUID:' + randomid + '\r\nREV:2007-04-10T10:19:03Z\r\nEND:VCARD\x00'
		#i += 1
	print 'Mostrando resultado final'
	for k in db.keys():
		print 'Clave %s: %s' % (k,db[k])
		print '**********************************************************************'
	print 'Cerrando base de datos'
	db.close()

def get_outlook_addressbook(file = None):
	f = open (file,'r')
	pattern = re.compile('\W*(?P<mail>[\w\-\_\.]+@([\w\-]{2,}\.)*([\w\-]{2,}\.)[\w\-]{2,3}).*')
	ab = []
	for l in f.readlines():
		a = ''
		for c in l:
			if re.search('[\w\s\.@]',c): 
				if c == '0': c = ' '
				a += c
		for t in a.split(' '):
			r = pattern.match(t)
			try:
				n = r.group('mail')
				if not n in ab: ab.append(n)
			except:
				pass
	f.close()
	for m in ab:
		print 'Revisando ' + m
		for n in ab:
			print n
			if m.endswith(n) and len(n) < len(m):
				print 'Sustituyendo %s por %s' % (m, n)
				ab.remove(m)
				break;
			elif n.endswith(m) and len(m) < len(n):
				print 'Sustituyendo %s por %s' % (n, m)
				ab.remove(n)
				break;
	return ab

def generate_csv(l):
	try:
		csv = open(os.path.join(os.path.expanduser('~'), 'contactos_Outlook.csv'),'w')
	except:
		print "No se pudo generar los contactos"
	else:
		for ad in l:
			csv.write(",,,,%s,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n"%ad)
		csv.close()

def generate_html(l):
	try:
		html = open(os.path.join(os.path.expanduser('~'), 'Mis contactos.html'),'w')
	except:
		print "No se pudo generar los contactos en html"
	else:
		html.write('<html>\n<head>\n<title>Libreta de direcciones</title>\n<style type=\"text/css\">\nbody{\n\tbackground:#FFFFFF;\n\tmargin:0;\n\ttext-align:justify;\n\tfont-family:Arial, Helvetica;\n\tfont-size:16px;\n\tcolor: #555555;\n}\n\n#cab{\n\ttext-align:center;\n\tbackground:navy; \n\tposition:absolute;\n\tleft:150px;\t\n}\n#sombra2{\n\tbackground:navy; \n\tposition:relative;\n\tleft:374px;\n\ttop:115px;\t\n\twidth:232px;\n\theight:21px;\n\tfont-size:16px;\n\tcolor:#c3d9ff;\n\ttext-align:center;\n\tpadding-top:4px;\n\n}\n#cont2{\n\ttext-align:justify;\n\twidth:700px;\n\tmargin-left:150px;\n}\n\n#text{\n\tmargin-top:100px;\n\tmin-height:500px; \n\tpadding: 50px;\n\tbackground:#c3d9ff;\n}\n\n#navlist { list-style-image: url(/usr/share/pixmaps/mozilla-thunderbird-pm-menu.png); }\n\nli {\n\tfont-size: 1em;\n\ttext-align:center;\n\tpadding-right: 10px;\n\tpadding-left: 10px;\n\twidth: 500px;\n\tborder-bottom:solid 1px #ffffff;\n\ttext-align:center;\n}\n\na{\n\tcolor:grey;\n}\n\na:hover {\n\tcolor:navy;\n}\n\nli:hover {\n\tfont-size: 1.6em;\n}\n\nli:hover + li {\n\tfont-size: 1.3em;\n}\n\nli:hover + li + li{\n\tfont-size: 1.1em;\n}\n.spiffy{display:block}\n.spiffy *{\n  display:block;\n  height:1px;\n  overflow:hidden;\n  font-size:.01em;\n  background:#e8eef7}\n.spiffy1{\n  margin-left:3px;\n  margin-right:3px;\n  padding-left:1px;\n  padding-right:1px;\n  border-left:1px solid #d2e2fb;\n  border-right:1px solid #d2e2fb;\n  background:#dee8f9}\n.spiffy2{\n  margin-left:1px;\n  margin-right:1px;\n  padding-right:1px;\n  padding-left:1px;\n  border-left:1px solid #c6dbfe;\n  border-right:1px solid #c6dbfe;\n  background:#e0eaf8}\n.spiffy3{\n  margin-left:1px;\n  margin-right:1px;\n  border-left:1px solid #e0eaf8;\n  border-right:1px solid #e0eaf8;}\n.spiffy4{\n  border-left:1px solid #d2e2fb;\n  border-right:1px solid #d2e2fb}\n.spiffy5{\n  border-left:1px solid #dee8f9;\n  border-right:1px solid #dee8f9}\n.spiffyfg{\n  min-height: 445px;\n  background:#e8eef7}\n</style>\n</head>\n<body>\n\t<link rel=\"shotcut icon\" href=\"/usr/share/pixmaps/amigu/icon_paginacion.png\">\n\t<div id=\"cab\">\n\t\t<img src=\"/usr/share/pixmaps/amigu/cab_amigu.png\">\n\t</div>\n\t<div id=\"sombra2\">\n\t\t\tLIBRETA DE DIRECCIONES\n\t</div>\n\t\n\t<div id=\"cont2\">\n\t\t<div id=\"text\">\n\t\t\t\t<p >Estas son las direcciones de correo de su libreta de contactos de Outlook Express.<br>\n\t\t\t\tPulse sobre cualquiera de las direcciones para enviar un mensaje con su gestor de correo predeterminado.\n\t\t\t\t</p>\n\n\t\t\t\t\t<div>\n \t\t\t\t\t\t<b class=\"spiffy\">\n\t\t\t\t\t\t<b class=\"spiffy1\"><b></b></b>\n\t\t\t\t\t\t<b class=\"spiffy2\"><b></b></b>\n  \t\t\t\t\t\t<b class=\"spiffy3\"></b>\n\t\t\t\t\t\t<b class=\"spiffy4\"></b>\n\t\t\t\t\t\t<b class=\"spiffy5\"></b></b>\n\n\t\t\t\t\t\t<div class=\"spiffyfg\">\n\t\t\t\t\t\t<table>\n\t\t\t\t\t\t<td>\n\t\t\t\t\t\t\t<ul id=\"navlist\">\n\t\t\t\t\t\t\t')

		for ad in l:
			html.write("<li id=\"active\"><a href=\"mailto:%s\" id=\"current\">%s</a></li>" % (ad, ad))
			
		html.write('</ul>\n\t\t\t\t\t\t</td>\n\t\t\t\t\t\t</table>\n  \t\t\t\t\t\t</div>\n\n\t\t\t\t\t\t<b class=\"spiffy\">\n\t\t\t\t\t\t<b class=\"spiffy5\"></b>\n\t\t\t\t\t\t<b class=\"spiffy4\"></b>\n\t\t\t\t\t\t<b class=\"spiffy3\"></b>\n \t\t\t\t\t\t<b class=\"spiffy2\"><b></b></b>\n\t\t\t\t\t\t<b class=\"spiffy1\"><b></b></b></b>\n\t\t\t\t\t</div>\n\t \t</div>\n\t</div>\n\n</body>\n</html>\n')
		html.close()
