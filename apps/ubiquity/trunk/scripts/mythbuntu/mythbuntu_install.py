#!/usr/bin/python
# -*- coding: utf-8 -*-

# Copyright (C) 2005 Javier Carranza and others for Guadalinex
# Copyright (C) 2005, 2006 Canonical Ltd.
# Copyright (C) 2007-2008 Mario Limonciello for Mythbuntu
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import os
import install
import sys
import syslog
import errno
import debconf


sys.path.insert(0, '/usr/lib/ubiquity')

from install import InstallStepError
from ubiquity.components import language_apply, apt_setup, timezone_apply, \
                                clock_setup, console_setup_apply, \
                                usersetup_apply, hw_detect, check_kernels, \
                                mythbuntu_apply

from mythbuntu_common.lirc import LircHandler

class Install(install.Install):
    def __init__(self):
        """Initializes the Mythbuntu installer extra objects"""
        self.lirc=LircHandler()
        install.Install.__init__(self)

    def run(self):
        """Run the install stage: copy everything to the target system, then
        configure it as necessary."""

        self.db.progress('START', 0, 100, 'ubiquity/install/title')
        self.db.progress('INFO', 'ubiquity/install/mounting_source')

        try:
            if self.source == '/var/lib/ubiquity/source':
                self.mount_source()

            self.db.progress('SET', 1)
            self.db.progress('REGION', 1, 75)
            try:
                self.copy_all()
            except EnvironmentError, e:
                if e.errno in (errno.ENOENT, errno.EIO, errno.EFAULT,
                               errno.ENOTDIR, errno.EROFS):
                    if e.filename is None:
                        error_template = 'cd_hd_fault'
                    elif e.filename.startswith('/target'):
                        error_template = 'hd_fault'
                    else:
                        error_template = 'cd_fault'
                    error_template = ('ubiquity/install/copying_error/%s' %
                                      error_template)
                    self.db.subst(error_template, 'ERROR', str(e))
                    self.db.input('critical', error_template)
                    self.db.go()
                    # Exit code 3 signals to the frontend that we have
                    # handled this error.
                    sys.exit(3)
                else:
                    raise

            self.db.progress('SET', 75)
            self.db.progress('REGION', 75, 76)
            self.db.progress('INFO', 'ubiquity/install/locales')
            self.configure_locales()

            self.db.progress('SET', 76)
            self.db.progress('REGION', 76, 77)
            self.db.progress('INFO', 'ubiquity/install/user')
            self.configure_user()

            self.db.progress('SET', 77)
            self.db.progress('REGION', 77, 78)
            self.run_target_config_hooks()

            self.db.progress('SET', 78)
            self.db.progress('REGION', 78, 79)
            self.db.progress('INFO', 'ubiquity/install/network')
            self.configure_network()

            self.db.progress('SET', 79)
            self.db.progress('REGION', 79, 80)
            self.db.progress('INFO', 'ubiquity/install/apt')
            self.configure_apt()

            self.db.progress('SET', 80)
            self.db.progress('REGION', 80, 85)
            self.db.progress('INFO', 'ubiquity/install/mythbuntu')
            self.configure_mythbuntu()

            self.db.progress('SET', 85)
            self.db.progress('REGION', 85, 86)
            self.db.progress('INFO', 'ubiquity/install/timezone')
            self.configure_timezone()

            self.db.progress('SET', 86)
            self.db.progress('REGION', 86, 87)
            self.db.progress('INFO', 'ubiquity/install/keyboard')
            self.configure_keyboard()

            self.db.progress('SET', 88)
            self.db.progress('REGION', 88, 89)
            self.remove_unusable_kernels()

            self.db.progress('SET', 89)
            self.db.progress('REGION', 89, 92)
            self.db.progress('INFO', 'ubiquity/install/hardware')
            self.configure_hardware()

            self.db.progress('SET', 92)
            self.db.progress('REGION', 92, 93)
            self.db.progress('INFO', 'ubiquity/install/bootloader')
            self.configure_bootloader()

            self.db.progress('SET', 93)
            self.db.progress('REGION', 93, 95)
            self.db.progress('INFO', 'ubiquity/install/installing')
            self.add_drivers_services()
            self.enable_cdrom()
            self.install_extras()

            self.db.progress('SET', 95)
            self.db.progress('REGION', 95, 96)
            self.db.progress('INFO', 'ubiquity/install/drivers')
            self.configure_drivers()

            self.db.progress('SET', 96)
            self.db.progress('INFO', 'ubiquity/install/services')
            self.configure_services()

            self.db.progress('SET', 96)
            self.db.progress('INFO', 'ubiquity/install/ir')
            self.configure_ir()

            self.db.progress('SET', 97)
            self.db.progress('REGION', 97, 99)
            self.db.progress('INFO', 'ubiquity/install/removing')
            self.remove_extras()

            self.db.progress('SET', 99)
            self.db.progress('INFO', 'ubiquity/install/log_files')
            self.copy_logs()

            self.db.progress('SET', 100)
        finally:
            self.cleanup()
            try:
                self.db.progress('STOP')
            except (KeyboardInterrupt, SystemExit):
                raise
            except:
                pass

    def enable_cdrom(self):
        """Adds the cdrom repository to the target installation"""
        self.chrex('apt-cdrom','add')

    def configure_mythbuntu(self):
        """Sets up mythbuntu items such as the initial database and username/password for mythtv user"""
        control = mythbuntu_apply.MythbuntuApply(None,self.db)
        #process package removal lists
        ret = control.run()
        if ret != 0:
            raise InstallStepError("MythbuntuApply Package List Generation failed with code %d" % ret)
        #process mythtv debconf info to be xfered
        ret = control.run_command(auto_process=True)
        if ret != 0:
            raise InstallStepError("MythbuntuApply Debconf Xfer failed with code %d" % ret)

    def add_drivers_services(self):
        """Installs Additional Drivers, Services & Firmware"""
        video_driver = self.db.get('mythbuntu/video_driver')
        vnc = self.db.get('mythbuntu/vncservice')
        nfs = self.db.get('mythbuntu/nfsservice')
        xmltv = self.db.get('mythbuntu/xmltv')
        dvbutils = self.db.get('mythbuntu/xmltv')
        hdhomerun = self.db.get('mythbuntu/hdhomerun')
        to_install = []
        to_remove = set()
        if video_driver == "nvidia_new":
            to_install.append('nvidia-glx-new')
        elif video_driver == "nvidia":
            to_install.append('nvidia-glx')
        elif video_driver == "nvidia_legacy":
            to_install.append('nvidia-glx-legacy')
        elif video_driver == "fglrx":
            to_install.append('xorg-driver-fglrx')
        elif video_driver == "pvr_350":
            to_install.append('xserver-xorg-video-ivtv')
        if vnc == 'true':
            to_install.append('x11vnc')
        if nfs == 'true':
            to_install.append('nfs-kernel-server')
            to_install.append('portmap')
        if xmltv == 'true':
            to_install.append('xmltv')
        if dvbutils == 'true':
            to_install.append('dvb-utils')
        if hdhomerun == 'true':
            to_install.append('hdhomerun-config')

        #Remove any conflicts before installing new items
        if to_remove != []:
            self.do_remove(to_remove)
        #Install new items
        self.record_installed(to_install)

    def configure_drivers(self):
        """Activates any necessary driver configuration"""
        control = mythbuntu_apply.AdditionalDrivers(None,self.db)
        ret = control.run_command(auto_process=True)
        if ret != 0:
            raise InstallStepError("Additional Driver Configuration failed with code %d" % ret)

    def configure_ir(self):
        """Configures the remote & transmitter per user choices"""
        #configure lircd for remote and transmitter
        ir_device={"modules":"","driver":"","device":"","lircd_conf":"","remote":"","transmitter":""}
        self.chroot_setup()
        self.chrex('dpkg-divert', '--package', 'ubiquity', '--rename',
                   '--quiet', '--add', '/sbin/udevd')
        try:
            os.symlink('/bin/true', '/target/sbin/udevd')
        except OSError:
            pass

        try:
            ir_device["remote"] = self.db.get('lirc/remote')
            self.set_debconf('lirc/remote',ir_device["remote"])
            if ir_device["remote"] == "Custom":
                ir_device["modules"] = self.db.get('lirc/remote_modules')
                ir_device["driver"] = self.db.get('lirc/remote_driver')
                ir_device["device"] = self.db.get('lirc/remote_device')
                ir_device["lircd_conf"] = self.db.get('lirc/remote_lircd_conf')
                self.set_debconf('lirc/remote_modules',ir_device["modules"])
                self.set_debconf('lirc/remote_driver',ir_device["driver"])
                self.set_debconf('lirc/remote_device',ir_device["device"])
                self.set_debconf('lirc/remote_lircd_conf',ir_device["lircd_conf"])
            else:
                ir_device["modules"] = ""
                ir_device["driver"] = ""
                ir_device["device"] = ""
                ir_device["lircd_conf"] = ""
            self.lirc.set_device(ir_device,"remote")
        except debconf.DebconfError:
            pass

        try:
            ir_device["transmitter"] = self.db.get('lirc/transmitter')
            self.set_debconf('lirc/transmitter',ir_device["transmitter"])
            if ir_device["transmitter"] == "Custom":
                ir_device["modules"] = self.db.get('lirc/transmitter_modules')
                ir_device["driver"] = self.db.get('lirc/transmitter_driver')
                ir_device["device"] = self.db.get('lirc/transmitter_device')
                ir_device["lircd_conf"] = self.db.get('lirc/transmitter_lircd_conf')
                self.set_debconf('lirc/transmitter_modules',ir_device["modules"])
                self.set_debconf('lirc/transmitter_driver',ir_device["driver"])
                self.set_debconf('lirc/transmitter_device',ir_device["device"])
                self.set_debconf('lirc/transmitter_lircd_conf',ir_device["lircd_conf"])
            else:
                ir_device["modules"] = ""
                ir_device["driver"] = ""
                ir_device["device"] = ""
                ir_device["lircd_conf"] = ""
            self.lirc.set_device(ir_device,"transmitter")
        except debconf.DebconfError:
            pass

        self.lirc.write_hardware_conf('/target/etc/lirc/hardware.conf')

        try:
            self.reconfigure('lirc')
        finally:
            try:
                os.unlink('/target/sbin/udevd')
            except OSError:
                pass
            self.chrex('dpkg-divert', '--package', 'ubiquity', '--rename',
                       '--quiet', '--remove', '/sbin/udevd')
        self.chroot_cleanup()

        #configure lircrc
        home = '/target/home/' + self.db.get('passwd/username')
        os.putenv('HOME',home)
        self.lirc.create_lircrc("/target/etc/lirc/lircd.conf",False)
        os.system('chown 1000:1000 -R ' + home)

    def configure_services(self):
        """Activates any necessary service configuration"""
        vnc = self.db.get('mythbuntu/vncservice')
        #vnc4server is broke in hardy.  Use x11vnc instead
        #if vnc == 'true':
        #    handler = mythbuntu_apply.VNCHandler('/target')
        #    handler.run()
        control = mythbuntu_apply.AdditionalServices(None,self.db)
        ret = control.run_command(auto_process=True)
        if ret != 0:
            raise InstallStepError("Additional Service Configuration failed with code %d" % ret)

    def remove_extras(self):
        """Try to remove packages that are installed on the live CD but not on
        the installed system."""
        # Looking through files for packages to remove is pretty quick, so
        # don't bother with a progress bar for that.
        # Check for packages specific to the live CD.
        # Also check for packages that need to be removed based upon the choices made during installation
        # This file (/tmp/filesystem.manifest-mythbuntu) will be created during the summary step
        if (os.path.exists("/tmp/filesystem.manifest-mythbuntu") and
            os.path.exists("/cdrom/casper/filesystem.manifest")):
            desktop_packages = set()
            manifest = open("/tmp/filesystem.manifest-mythbuntu")
            for line in manifest:
                if line.strip() != '' and not line.startswith('#'):
                    desktop_packages.add(line.split()[0])
            manifest.close()
            live_packages = set()
            manifest = open("/cdrom/casper/filesystem.manifest")
            for line in manifest:
                if line.strip() != '' and not line.startswith('#'):
                    live_packages.add(line.split()[0])
            manifest.close()
            difference = live_packages - desktop_packages
        else:
            difference = set()

        # Keep packages we explicitly installed.
        difference -= self.query_recorded_installed()

        if len(difference) == 0:
            return

        # Don't worry about failures removing packages; it will be easier
        # for the user to sort them out with a graphical package manager (or
        # whatever) after installation than it will be to try to deal with
        # them automatically here.
        self.do_remove(difference)

if __name__ == '__main__':
    if not os.path.exists('/var/lib/ubiquity'):
        os.makedirs('/var/lib/ubiquity')
    if os.path.exists('/var/lib/ubiquity/install.trace'):
        os.unlink('/var/lib/ubiquity/install.trace')

    install = Install()
    sys.excepthook = install.excepthook
    install.run()
    sys.exit(0)
