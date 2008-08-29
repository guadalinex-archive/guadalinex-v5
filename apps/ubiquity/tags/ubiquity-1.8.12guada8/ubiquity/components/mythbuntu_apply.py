# -*- coding: UTF-8 -*-

# Written by Mario Limonciello <superm1@ubuntu.com>.
# Copyright (C) 2007-2008 Mario Limonciello
# Copyright (C) 2007 Jared Greenwald
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
import sys
import locale

#Workaround for bugs 149935 and 150029
os.environ['LC_CTYPE']='C'

import xorgconfig

import re
import string
from ubiquity.filteredcommand import FilteredCommand

class MythbuntuApply(FilteredCommand):
    def prepare(self):
        return (['/usr/share/ubiquity/apply-type', '/target'],
                [])

    def run(self):
        out_f = open("/tmp/filesystem.manifest-mythbuntu", 'w')
        in_f = open("/cdrom/casper/filesystem.manifest-desktop")
        patternline = "^mythbuntu-live|^expect|^tcl8.4"
        installtype = self.db.get('mythbuntu/install_type')
        if installtype == "Slave Backend/Frontend":
            patternline += "|^mythtv-backend-master|^mythtv-database|^mysql-server-5.0|^mysql-server|^mythtv\ "
        elif installtype == "Master Backend":
            patternline += "|^mythtv-frontend|^mythtv\ "
        elif installtype == "Slave Backend":
            patternline += "|^mythtv-backend-master|^mythtv-database|^mysql-server-5.0|^mythtv-frontend|^mythtv\ "
        elif installtype == "Frontend":
            patternline += "|^mythtv-backend-master|^mythtv-database|^mythtv-backend|^mysql-server-5.0|^mysql-server|^mythtv\ "
        mytharchive = self.db.get('mythbuntu/mytharchive')
        if mytharchive == "false":
            patternline += "|^mytharchive|^ffmpeg|^genisoimage|^dvdauthor|^mjpegtools|^dvd+rw-tools|^python-imaging|^python-mysqldb"
        mythbrowser = self.db.get('mythbuntu/mythbrowser')
        if mythbrowser == "false":
            patternline += "|^kdelibs4c2a|^mythbrowser"
        mythcontrols = self.db.get('mythbuntu/mythcontrols')
        if mythcontrols == "false":
            patternline += "|^mythcontrols"
        mythflix = self.db.get('mythbuntu/mythflix')
        if mythflix == "false":
            patternline += "|^mythflix"
        mythgallery = self.db.get('mythbuntu/mythgallery')
        if mythgallery == "false":
            patternline += "|^mythgallery"
        mythgame = self.db.get('mythbuntu/mythgame')
        if mythgame == "false":
            patternline += "|^mythgame"
        mythmovies = self.db.get('mythbuntu/mythmovies')
        if mythmovies == "false":
            patternline += "|^mythmovies"
        mythmusic = self.db.get('mythbuntu/mythmusic')
        if mythmusic == "false":
            patternline += "|^mythmusic|^fftw2|^libcdaudio1|^libfaad2-0|^libflac8"
        mythnews = self.db.get('mythbuntu/mythnews')
        if mythnews == "false":
            patternline += "|^mythnews"
        mythphone = self.db.get('mythbuntu/mythphone')
        if mythphone == "false":
            patternline += "|^mythphone"
        mythstream = self.db.get('mythbuntu/mythstream')
        if mythstream == "false":
            patternline += "|^mythstream"
        mythvideo = self.db.get('mythbuntu/mythvideo')
        if mythvideo == "false":
            patternline += "|^mythvideo|^libwww-perl|^libxml-simple-perl"
        mythweather = self.db.get('mythbuntu/mythweather')
        if mythweather == "false":
            patternline += "|^mythweather"
        mythweb = self.db.get('mythbuntu/mythweb')
        if mythweb == "false":
            patternline += "|^apache2|^libapache2|^php|^mythweb"
        official = self.db.get('mythbuntu/officialthemes')
        if official != "":
            for theme in string.split(official," "):
                if theme != "":
                    patternline += "|^" + theme
        community = self.db.get('mythbuntu/communitythemes')
        if community != "":
            for theme in string.split(community," "):
                if theme != "":
                    patternline += "|^" + theme
        samba = self.db.get('mythbuntu/sambaservice')
        if samba == "false":
            patternline += "|^samba|^samba-common|^smbfs"
        vnc = self.db.get('mythbuntu/vncservice')
        if vnc == "false":
            patternline += "|^vnc4-common|^x11vnc"
        ssh = self.db.get('mythbuntu/sshservice')
        if ssh == "false":
            patternline += "|^openssh-server"
        hdhomerun = self.db.get('mythbuntu/hdhomerun')
        if hdhomerun == "false":
            patternline += "|^hdhomerun-config"
        xmltv = self.db.get('mythbuntu/xmltv')
        if xmltv == "false":
            patternline += "|^xmltv"
        dvbutils = self.db.get('mythbuntu/dvbutils')
        if dvbutils == "false":
            patternline += "|^dvbutils"
        pattern = re.compile(patternline)
        for line in in_f:
            if pattern.search(line) is None:
                out_f.write(line)
        in_f.close()
        out_f.close()
        return 0

class AdditionalDrivers(FilteredCommand):
    def prepare(self):
        return (['/usr/share/ubiquity/apply-drivers', '/target'],[])

class VNCHandler:
    """Used to properly enable VNC in a target configuration"""

    # rather ugly workaround for lp: #136482)
    locale.setlocale(locale.LC_ALL, 'C')

    def __init__(self,root):
        self.add_modules = ["vnc"]
        self.add_screen = [ ['SecurityTypes', 'VncAuth'], ['UserPasswdVerifier', 'VncAuth'], ['PasswordFile', '/root/.vnc/passwd']]
        self.root = root

        try:
            self.xorg_conf = xorgconfig.readConfig(root + '/etc/X11/xorg.conf')
        except (IOError, xorgconfig.ParseException, AttributeError):
            self.xorg_conf = None

    def run(self):
        """Adds necessary lines for enabling VNC upon the next boot"""

        # backup the current xorg.conf
        open(os.path.join(self.root + "/etc/X11/xorg.conf.oldconf"), "w").write(open(self.root + '/etc/X11/xorg.conf').read())

        have_modules = len(self.xorg_conf.getSections("module")) > 0
        if self.add_modules:
            if not have_modules:
                self.xorg_conf.append(self.xorg_conf.makeSection(None, ["Section",
                    "Module"]))
            for m in self.add_modules:
                self.xorg_conf.getSections("module")[0].addModule(m)

        screen_opts=self.xorg_conf.getSections("screen")[0].option
        for item in self.add_screen:
            screen_opts.append(screen_opts.makeLine(None,item))

        self.xorg_conf.writeConfig(self.root + '/etc/X11/xorg.conf')

class AdditionalServices(FilteredCommand):
    def prepare(self):
        return (['/usr/share/ubiquity/apply-services', '/target'],[])
