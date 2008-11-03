#!/usr/bin/env python
#-*- coding: utf8 -*-

#Módulo certmanager- Módulo que localiza certificados digitales en dispositivos
# de volumen y que configura un conjunto de aplicaciones para que funcionen
# correctamente
#
#Copyright (C) 2005 Junta de Andalucía
#
#Autor/es (Author/s):
#
#- Lorenzo Gil Sanchez <lgs@yaco.es>
#
#Contribuciones:
#
#- J. Félix Ontañón <fontanon@emergya.es>
#
#Este fichero es parte del módulo E-Admin de Guadalinex 2006
#
#E-Admin de Guadalinex 2006 es software libre. Puede redistribuirlo y/o modificarlo
#bajo los términos de la Licencia Pública General de GNU según es
#publicada por la Free Software Foundation, bien de la versión 2 de dicha
#Licencia o bien (según su elección) de cualquier versión posterior.
#
#Detección de Hardware de Guadalinex 2005  se distribuye con la esperanza de que sea útil,
#pero SIN NINGUNA GARANTÍA, incluso sin la garantía MERCANTIL
#implícita o sin garantizar la CONVENIENCIA PARA UN PROPÓSITO
#PARTICULAR. Véase la Licencia Pública General de GNU para más detalles.
#
#Debería haber recibido una copia de la Licencia Pública General
#junto con Detección de Hardware de Guadalinex 2005 . Si no ha sido así, escriba a la Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
#
#-------------------------------------------------------------------------
#
#This file is part of E-Admin de Guadalinex 2006 .
#
#E-Admin de Guadalinex 2006  is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#at your option) any later version.
#
#E-Admin Guadalinex 2006  is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Foobar; if not, write to the Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import commands
import glob
import optparse
import os
import subprocess
import tempfile
import exceptions
import ConfigParser

import gobject
import gtk

FIREFOX_CMD  = '/usr/bin/firefox'
CERTUTIL_CMD = '/usr/bin/certutil'
MODUTIL_CMD  = '/usr/bin/modutil'
PK12UTIL_CMD = '/usr/bin/pk12util'
SIEMENS_ATR = "3B F2 98 00 FF C1 10 31 FE 55 C8 04 12"

class FireFoxSecurityUtils(object):
    def is_firefox_running(self):
        cmd = '%s -remote "ping()"' % FIREFOX_CMD
        status, output = commands.getstatusoutput(cmd)
        return status == 0

    def create_default_profile(self):
        cmd = '%s -CreateProfile "default"' % FIREFOX_CMD
        status, output = commands.getstatusoutput(cmd)
        return status == 0

    def get_default_profile_dir(self):
        user_dir = os.path.expanduser('~')
        ff_dir = os.path.join(user_dir, '.mozilla', 'firefox')
	if not os.path.exists(ff_dir):
            return

        config = ConfigParser.ConfigParser()
        config.readfp(file(os.path.join(ff_dir, 'profiles.ini')))
        profiles = [section for section in config.sections() \
                    if config.has_option(section, 'Name')]

        if len(profiles) == 1:
            default = profiles[0]
        else:
            for profile in profiles:
                if (config.has_option(profile, 'Default') and
                    config.get(profile, 'Default') == '1'):
                    default = profile

        path = config.get(default, 'Path')
        return os.path.join(ff_dir, path)

    def has_security_method(self, security_method):
        profile = self.get_default_profile_dir()
        if not profile:
            return False

        cmd = '%s -list -dbdir "%s"' % (MODUTIL_CMD, profile)
        status, output = commands.getstatusoutput(cmd)
        return security_method in output

    def add_security_method(self, name, library, mechanisms="FRIENDLY"):
        profile = self.get_default_profile_dir()
        if not profile:
            return False

        # we need to use the subprocess module since this command request
        # interactive input
        cmd = [MODUTIL_CMD, '-add', "%s" % name, '-libfile', library,
               '-dbdir', "%s" % profile]
        process = subprocess.Popen(cmd,
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.stdin.write('\n')
        process.wait()
        return process.returncode == 0

    def has_root_ca_certificate(self, ca_name):
        profile = self.get_default_profile_dir()
        if not profile:
            return False

        cmd = '%s -L -n "%s" -d "%s"' % (CERTUTIL_CMD, ca_name, profile)
        status, output = commands.getstatusoutput(cmd)
        return status == 0

    def add_root_ca_certificate(self, ca_name, certificate_file):
        profile = self.get_default_profile_dir()
        if not profile:
            return False

        if self.has_root_ca_certificate(ca_name):
            return False

        cmd = '%s -A -a -d "%s" -i %s -n "%s" -t "TCu,Cu,Cu"'
        cmd = cmd % (CERTUTIL_CMD, profile, certificate_file, ca_name)
        status, output = commands.getstatusoutput(cmd)
        return status == 0

    def add_user_certificate(self, certificate_file, password):
        profile = self.get_default_profile_dir()
        if not profile:
            return False

        fd, password_file = tempfile.mkstemp(text=True)
        os.write(fd, password)
        os.close(fd)

	cmd = '%s -i "%s" -d "%s" -w "%s"'
        cmd = cmd % (PK12UTIL_CMD, certificate_file, profile, password_file)
	status, output = commands.getstatusoutput(cmd)
	os.unlink(password_file)
	return status == 0

    def remove_user_certificate(self, certificate_name):
        profile = self.get_default_profile_dir()
        if not profile:
            return False

        cmd = '%s -D -d "%s" -n "%s"'
        cmd = cmd % (CERTUTIL_CMD, profile, certificate_name)
        status, output = commands.getstatusoutput(cmd)
        return status == 0

    def remove_security_method(self, name):
        profile = self.get_default_profile_dir()
        if not profile:
            return False

        # we need to use the subprocess module since this command request
        # interactive input
        cmd = [MODUTIL_CMD, '-delete', "%s" % name, '-dbdir', "%s" % profile]
        process = subprocess.Popen(cmd,
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.stdin.write('\n')
        process.wait()
        return process.returncode == 0

    def list_certificates(self):
        profile = self.get_default_profile_dir()
        if not profile:
            return False

        cmd = '%s -L -d "%s"' % (CERTUTIL_CMD, profile)
        status, output = commands.getstatusoutput(cmd)
        if status == 0:
            certificates = output.split('\n')
            return certificates

DNIE_ROOT_CERT_NAME = "AC RAIZ DNIE - DIRECCION GENERAL DE LA POLICIA"
DNIE_ROOT_CERT_FILE = "/usr/share/opensc-dnie/ac_raiz_dnie.crt"
FNMT_ROOT_CERT_NAME = "FNMT Clase 2 CA - FNMT"
FNMT_ROOT_CERT_FILE = "/usr/share/ca-certificates/fnmt/FNMTClase2CA.crt"
FNMT_LICENSE        = "/usr/share/ca-certificates/fnmt/condiciones_uso.txt"
DNIE_PKCS11_LIB     = "/usr/lib/opensc-pkcs11.so"
CERES_PKCS11_LIB    = "/usr/lib/opensc-pkcs11.so"
SIEMENS_PKCS11_LIB  = "/usr/local/lib/libsiecap11.so"

class LicenseDialog(gtk.Dialog):
    def __init__(self):
        gtk.Dialog.__init__(self,
                            "Aceptación de Licencia de Usuario Final",
                            None, gtk.DIALOG_NO_SEPARATOR | gtk.DIALOG_MODAL,
                            (gtk.STOCK_NO, gtk.RESPONSE_REJECT,
                            gtk.STOCK_YES, gtk.RESPONSE_ACCEPT))

        self.set_position(gtk.WIN_POS_CENTER)
	self.set_default_size(500,400)
        self.set_border_width(12)
	self.vbox.set_spacing(6)

	title = gtk.Label('<b>Es necesario instalar el certificado raiz de la FNMT</b>')
	title.set_use_markup(True)
	title.set_property('xalign', 0.0)
	self.vbox.pack_start(title, False, False)

	license_box = gtk.TextView()
	license_box.set_left_margin(6)
	license_box.set_right_margin(6)
	license_box.set_editable(False)
	license_box.set_cursor_visible(False)
	license_box.set_wrap_mode(gtk.WRAP_WORD)
	license = file(FNMT_LICENSE).read()
	license_box.get_buffer().set_text(license)

	sw = gtk.ScrolledWindow()
	sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	sw.set_shadow_type(gtk.SHADOW_IN)
	sw.add(license_box)

	self.vbox.pack_start(sw, True, True)

	question = gtk.Label('<b>¿Acepta los términos de esta licencia?</b>')
	question.set_use_markup(True)
	question.set_property('xalign', 0.0)
	self.vbox.pack_start(question, False, False)

	self.vbox.show_all()

class Application(object):
    """Base abstract class Application that can use a certificate"""

    def __init__(self, name):
        self._name = name

    def setup(self,
              user_certificates=[],
              install_dnie=False,
              install_ceres=False):
        """This method should be overriden in subclasses

        It returns the success state of the process
        """
        return True

    def remove_certificates(self, certs):
        pass

    def _wait_for_running_instances(self):
        dialog = gtk.MessageDialog(None, 0, gtk.MESSAGE_INFO,
                                   gtk.BUTTONS_NONE)
        next_btn = dialog.add_button(gtk.STOCK_GO_FORWARD, gtk.RESPONSE_ACCEPT)
        next_btn.set_sensitive(False)
        dialog.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        dialog.set_title('Configurando %s' % self._name)
        dialog.set_position(gtk.WIN_POS_CENTER)
        dialog.set_markup('Debe cerrar todas las ventanas de %s para configurar los certificados digitales' % self._name)
        progress = gtk.ProgressBar()
        progress.set_text('Esperando a que finalice %s' % self._name)
        progress.set_pulse_step(0.1)
        progress.pulse()
        dialog.vbox.pack_start(progress, False, False)
        dialog.show_all()
        gobject.timeout_add(300, self._check_app_running, progress, next_btn)
        result = dialog.run()
        dialog.destroy()
        return result == gtk.RESPONSE_ACCEPT

    def _check_app_running(self, progress, next_btn):
        retval = self._is_app_running()
        if retval:
            progress.pulse()
        else:
            progress.set_text('%s ha finalizado' % self._name)
            progress.set_fraction(1.0)
            next_btn.set_sensitive(True)

        return retval

    def _is_app_running(self):
        """Subclasses should override this True/False method"""

    def _ask_permission(self):
	dialog = LicenseDialog()
	result = dialog.run()
	dialog.destroy()
	return result == gtk.RESPONSE_ACCEPT

class FireFoxApp(Application):

    def __init__(self, name='FireFox'):
        super(FireFoxApp, self).__init__(name)
        self._ff = FireFoxSecurityUtils()

    def _is_app_running(self):
        return self._ff.is_firefox_running()

    def _is_siemens_supported(self):
	return os.path.exists(SIEMENS_PKCS11_LIB)

    def setup(self,
              user_certificates=[],
              install_dnie=False,
              install_ceres=False):
        # check that we have the root certificates of relevant spanish agencies
        has_fnmt_cert = self._ff.has_root_ca_certificate(FNMT_ROOT_CERT_NAME)
        has_dnie_cert = self._ff.has_root_ca_certificate(DNIE_ROOT_CERT_NAME)

        if self._ff.get_default_profile_dir() is None:
            self._ff.create_default_profile()

        if ((not has_fnmt_cert and (user_certificates or install_ceres)) or
            (not has_dnie_cert and install_dnie)):
            # install the root certificates stopping Firefox if needed

            if self._is_app_running():
                abort = not self._wait_for_running_instances()
                if abort:
		    print "WE1"
                    return False, ''

            if not has_fnmt_cert and (user_certificates or install_ceres):
                if True == self._ask_permission():
                    self._ff.add_root_ca_certificate(FNMT_ROOT_CERT_NAME,
                                                     FNMT_ROOT_CERT_FILE)
                else:
		    print "WE2"
                    return False, ''

            if not has_dnie_cert and install_dnie:
                self._ff.add_root_ca_certificate(DNIE_ROOT_CERT_NAME,
                                                 DNIE_ROOT_CERT_FILE)

        if self._is_app_running():
            abort = not self._wait_for_running_instances()
            if abort:
                return False, ''

        if install_dnie or install_ceres:
	    try:
	        card_types = self.check_smartcards()
	    except FireFoxAppError, e:
	        print e.errno, e.desc
	        return False, ''

	    if SIEMENS_ATR in card_types and self._is_siemens_supported():
	        print "Detected Siemens Smartcard"
                if self._ff.has_security_method('Tarjeta Inteligente'):
	   	    self._ff.remove_security_method('Tarjeta Inteligente')

                if self._ff.has_security_method('DNIe'):
		    self._ff.remove_security_method('DNIe')

	        if install_ceres and not self._ff.has_security_method('Tarjeta Inteligente Siemens'):
                    self._ff.add_security_method('Tarjeta Inteligente Siemens', SIEMENS_PKCS11_LIB)
	    else:
	        print "Detected FNMT-smartcard, DNIe or other"
	        if self._ff.has_security_method('Tarjeta Inteligente Siemens'):
		    self._ff.remove_security_method('Tarjeta Inteligente Siemens')

	        if install_dnie and not self._ff.has_security_method('DNIe'):
                    self._ff.add_security_method('DNIe', DNIE_PKCS11_LIB)

                if install_ceres and not self._ff.has_security_method('Tarjeta Inteligente'):
                    self._ff.add_security_method('Tarjeta Inteligente', CERES_PKCS11_LIB)

        # install the user certificates
        success = True
        new_certificates = []
        for cert in user_certificates:
            success = success and self._install_certificate(cert,
                                                            new_certificates)

        return success, new_certificates

    def _install_certificate(self, certificate, new_certificates):
        attempts = 0
        valid = False
        print_warning = False
        certificates_before = self._ff.list_certificates()
        while attempts < 3 and not valid:
            password = self._ask_for_password(certificate, print_warning)
            if password:
                valid = self._ff.add_user_certificate(certificate, password)
                attempts += 1
                print_warning = True
            else:
                return False # User cancel

        if not valid:
            msg = 'No fue posible agregar el certificado %s porque la contraseña no es válida' % certificate
            dialog = gtk.MessageDialog(None, 0, gtk.MESSAGE_ERROR,
                                       gtk.BUTTONS_CLOSE, msg)
            dialog.set_title('Error configurando %s' % self._name)
            dialog.set_position(gtk.WIN_POS_CENTER)
            dialog.run()
            dialog.destroy()
        else:
            certificates_after = self._ff.list_certificates()
            new_certificate = self._get_new_certificate(certificates_before,
                                                        certificates_after)
            if new_certificate:
                new_certificates.append(new_certificate)

        return valid

    def _ask_for_password(self, certificate, warn_user=False):
        dialog = gtk.MessageDialog(None, 0, gtk.MESSAGE_INFO,
                                   gtk.BUTTONS_OK_CANCEL)
        dialog.set_title('Configurando %s' % self._name)
        dialog.set_position(gtk.WIN_POS_CENTER)
        dialog.set_default_response(gtk.RESPONSE_OK)
        dialog.set_markup('Introduzca la contraseña para desbloquear el certificado situado en el fichero <b>%s</b>' % certificate)
        if warn_user:
            dialog.format_secondary_text('La contraseña anterior no es válida')
        entry = gtk.Entry()
        entry.set_activates_default(True)
        entry.set_visibility(False) # this entry is for passwords
        entry.show()
        parent = dialog.vbox.get_children()[0].get_children()[1]
        parent.pack_start(entry, False, False)
        result = dialog.run()
        retval = None
        if result == gtk.RESPONSE_OK:
            retval = entry.get_text()
        dialog.destroy()
        return retval

    def _get_new_certificate(self, certificates_before, certificates_after):
        def _filter(cert):
            """True if cert is a user certificate"""
            if 'u,u,u' in cert:
                return True
            return False

        users_before = [c for c in certificates_before if _filter(c)]
        users_after = [c for c in certificates_after if _filter(c)]
        new_cert = None
        for c in users_after:
            if c not in users_before:
                new_cert = c
                break

        if new_cert:
            new_cert = new_cert.replace('u,u,u', '')
            return new_cert.strip()

    def remove_certificates(self, certs):
        for cert in certs:
            self._ff.remove_user_certificate(cert)

    def check_smartcards(self):
	status, output = commands.getstatusoutput("getatr -a")
	if status != 0:
	    raise FireFoxAppError(0, "No se pudo acceder al lector de tarjetas")
	    return False

	card_types = []
	for smartcard_atr in filter(lambda x: x != "None", output.split('\n')):
	    if SIEMENS_ATR not in card_types:
		if smartcard_atr not in card_types:
		    card_types.append(smartcard_atr)
	    else:
		if smartcard_atr != SIEMENS_ATR:
		    raise FireFoxAppError(1, "No puede usar tarjeta Siemens y otra no Siemens simultaneamente")
  		    return False


	if card_types:
	    return card_types	
	else:
	    raise FireFoxAppError(2, "No se detecto tarjeta insertada")
	    return False


class FireFoxAppError(exceptions.Exception):
    def __init__(self, errno, desc):
	self.errno = errno
	self.desc = desc


class EvolutionApp(Application):
    """Firefox should be configured since Evolution depends on its security
    database for apropiate working"""

    def __init__(self, name='Evolution'):
        super(EvolutionApp, self).__init__(name)
        self._ff = FireFoxSecurityUtils()
        self._evo_dir = os.path.join(os.path.expanduser('~'), '.evolution')

    def _is_app_running(self):
        cmd = '/bin/ps -C evolution -o pid='
        status, output = commands.getstatusoutput(cmd)
        return status == 0

    def setup(self,
              user_certificates=[],
              install_dnie=False,
              install_ceres=False):
        """Link Evolution database to Firefox database"""

        if self._is_app_running():
            abort = not self._wait_for_running_instances()
            if abort:
                return False

        if self._ff.get_default_profile_dir() is None:
            self._ff.create_default_profile()

        ff_profile_dir = self._ff.get_default_profile_dir()

        if not os.path.exists(self._evo_dir):
            # We can create the evo dir and next time Evolution is executed
            # it will use such directory and add its own files
            os.mkdir(self._evo_dir)
        else:
            self._backup_evo_database()

        self._create_links(ff_profile_dir)

        return True, []

    def _backup_evo_database(self):
        for filename in ('cert8.db', 'key3.db', 'secmod.db'):
            src = os.path.join(self._evo_dir, filename)
            if os.path.exists(src):
                dst = src + '.org'
                os.rename(src, dst)

    def _create_links(self, ff_profile_dir):
        for filename in ('cert8.db', 'key3.db', 'secmod.db'):
            src = os.path.join(ff_profile_dir, filename)
            dst = os.path.join(self._evo_dir, filename)
            try:
                os.unlink(dst)
            except OSError:
                pass
            os.symlink(src, dst)

class CertManager(object):
    """Search for certificates in a specific path and configure applications
    to use the certificates that the user selects
    """

    # We serch for certificate files with this extensions
    known_extensions = ('cert', 'p12')

    def __init__(self, applications=[]):
        self._applications = applications

    def run(self, options):
        certs = []
        new_certificates = []
        if options.log_file is not None:
            old_certs = file(options.log_file, 'r').readlines()
            if old_certs:
                to_remove = self.select_certificates(old_certs, 'el sistema',
                                                     False)
                for app in self._applications:
                    app.remove_certificates(to_remove)

        if options.search_path is not None:
            search_path = options.search_path.replace('~', os.path.expanduser('~'))
            cert_list = self.search_certificates(search_path)

            if cert_list:
                certs = self.select_certificates(cert_list, search_path)

	success = True
        if options.install_dnie or options.install_ceres or certs:
            for app in self._applications:
                status, new_certs = app.setup(certs, 
					options.install_dnie, 
					options.install_ceres)
                success = success and status
                new_certificates += new_certs

            # Finish message
            what = []
            if options.install_dnie and certs:
                what.append('el DNIe')
            if options.install_ceres:
                what.append('los módulos CERES')
            if certs:
                what.append('los certificados de usuario')
            what = ', '.join(what)
            if success:
                msg = 'La instalacion de %s ha finalizado correctamente' % what
                icon = gtk.MESSAGE_INFO
            else:
                msg = 'Ocurrió un error durante la instalación de %s y no se ha completado correctamente' % what
                icon = gtk.MESSAGE_ERROR
            dialog = gtk.MessageDialog(None, 0, icon, gtk.BUTTONS_OK, msg)
            dialog.set_position(gtk.WIN_POS_CENTER)
            dialog.run()
            dialog.destroy()

        if new_certificates and options.search_path is not None:
            # Save a log file
            user_dir = os.path.expanduser('~')
            f = file(os.path.join(user_dir, '.certmanager.log'), 'w')
            f.write('\n'.join(new_certificates))

    def search_certificates(self, search_path):
        ret = []
        for extension in self.known_extensions:
            path = os.path.join(search_path, '*.%s' % extension)
            ret += glob.glob(path)

        return ret

    def select_certificates(self, cert_list, search_path, add=True):
        dialog = CertificatesDialog(search_path, cert_list, add)
        ret = []

        if gtk.RESPONSE_ACCEPT == dialog.run():
            ret = dialog.get_selected_certificates()
        dialog.destroy()

        return ret

class CertificatesDialog(gtk.Dialog):
    """Dialog to ask the user which certificates he/she wishes to add/remove"""
    def __init__(self, path, cert_list, add=True, parent=None):
        gtk.Dialog.__init__(self,
                            title="CertManager",
                            parent=parent,
                            flags=gtk.DIALOG_NO_SEPARATOR | gtk.DIALOG_MODAL,
                            buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                                     gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
        self.set_position(gtk.WIN_POS_CENTER)
        self.set_border_width(12)

        hbox = gtk.HBox()
        image = gtk.image_new_from_stock(gtk.STOCK_DIALOG_INFO,
                                         gtk.ICON_SIZE_DIALOG)
        image.set_alignment(0.0, 0.0)

        image.show()
        hbox.pack_start(image, False, False)

        vbox = gtk.VBox(spacing=6)

        # Information label
        info = 'Se han encontrado los siguientes\n certificados en %s' % path
        markup = '<span size="large" weight="bold">%s</span>' % info
        label = gtk.Label(markup)
        label.set_use_markup(True)
        label.set_alignment(0.0, 0.5)
        label.show()
        vbox.pack_start(label, False, False)

        # List with certicates
        model = gtk.ListStore(bool, str)
        for cer in cert_list:
            model.append((True, cer))

        self.treeview = gtk.TreeView(model)
        self.treeview.set_headers_visible(False)
        self.treeview.set_rules_hint(True)
        self.treeview.get_selection().set_mode(gtk.SELECTION_NONE)

        toggle_renderer = gtk.CellRendererToggle()
        toggle_renderer.connect('toggled', self._on_cert__toggled)
        column1 = gtk.TreeViewColumn('', toggle_renderer, active=0)
        self.treeview.append_column(column1)

        text_renderer = gtk.CellRendererText()
        column2 = gtk.TreeViewColumn('', text_renderer, text=1)
        self.treeview.append_column(column2)

        self.treeview.show()

        scrolled_window = gtk.ScrolledWindow()
        scrolled_window.add(self.treeview)
        scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrolled_window.set_shadow_type(gtk.SHADOW_IN)
        scrolled_window.show()
        vbox.pack_start(scrolled_window, True, True)

        # Request label
        if add:
            request = 'Seleccione aquellos certificados que desee utilizar'
        else:
            request = 'Seleccione aquellos certificados que desee eliminar'
        label = gtk.Label(request)
        label.set_alignment(0.0, 0.5)
        label.show()
        vbox.pack_start(label, False, False)

        vbox.show()
        hbox.pack_start(vbox, True, True)

        hbox.show()
        self.vbox.pack_start(hbox, True, True)

    def _on_cert__toggled(self, renderer, path):
        model = self.treeview.get_model()
        tree_iter = model.get_iter(path)
        oldvalue = model.get_value(tree_iter, 0)
        model.set_value(tree_iter, 0, not oldvalue)

    def get_selected_certificates(self):
        model = self.treeview.get_model()
        ret = []
        for element in model:
            if element[0]:
                ret.append(element[1])
        return ret

if __name__ == '__main__':
    user_dir = os.path.expanduser('~')
    witness_file = os.path.join(user_dir, '.certmanager.lock')

    parser = optparse.OptionParser()
    parser.add_option('-p', '--search-path',
                      dest='search_path',
                      default=None,
                      help='Where to search certificates')
    parser.add_option('-d', '--install-dnie',
                      action='store_true',
                      dest='install_dnie',
                      default=False,
                      help='Install necesary modules for the DNIe')
    parser.add_option('-c', '--install-ceres',
                      action='store_true',
                      dest='install_ceres',
                      default=False,
                      help='Install necesary modules for CERES cards')
    parser.add_option('-r', '--run-only-once',
                      action='store_true',
                      dest='run_only_once',
                      default=False,
                      help='Do not run if this command was executed before')
    parser.add_option('-s', '--sm-client-id',
                      dest='sm_client_id',
                      default=None,
                      help='Session Manager client id (not used so far)')
    parser.add_option('-u', '--uninstall',
                      dest='log_file',
                      default=None,
                      help='Remove user certificates in the log file')

    (options, args) = parser.parse_args()

    if not (options.run_only_once and os.path.exists(witness_file)):

        cert_manager = CertManager([FireFoxApp(), EvolutionApp()])
        cert_manager.run(options)

    if not os.path.exists(witness_file):
        f = file(witness_file, 'w')
        f.write('Remove this file to allow certmanager run again\n')
