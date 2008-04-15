# -*- coding: UTF-8 -*-

# Written by Mario Limonciello <superm1@ubuntu.com>.
# Copyright (C) 2007 Mario Limonciello
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

from ubiquity.filteredcommand import FilteredCommand

class MythbuntuAdvancedType(FilteredCommand):
    def prepare(self):
        questions = ['^mythbuntu/advanced_install']
        return (['/usr/share/ubiquity/ask-advanced'], questions)

    def run(self,priority,question):
        if question.startswith('mythbuntu/advanced_install'):
            advanced = self.frontend.get_advanced()
            self.preseed_bool('mythbuntu/advanced_install', advanced)
        return FilteredCommand.run(self, priority, question)

    def ok_handler(self):
        self.preseed_bool('mythbuntu/advanced_install', self.frontend.get_advanced())
        FilteredCommand.ok_handler(self)

class MythbuntuInstallType(FilteredCommand):
    def prepare(self):
        questions = ['^mythbuntu/install_type']
        return (['/usr/share/ubiquity/ask-type'], questions)

    def run(self,priority,question):
        if question.startswith('mythbuntu/install_type'):
            installtype = self.frontend.get_installtype()
            self.preseed('mythbuntu/install_type', installtype)
        return FilteredCommand.run(self, priority, question)

    def ok_handler(self):
        self.preseed('mythbuntu/install_type',self.frontend.get_installtype())
        FilteredCommand.ok_handler(self)

class MythbuntuPlugins(FilteredCommand):
    def prepare(self):
        questions = ['^mythbuntu/mytharchive',
             '^mythbuntu/mythbrowser',
             '^mythbuntu/mythcontrols',
             '^mythbuntu/mythflix',
             '^mythbuntu/mythgallery',
             '^mythbuntu/mythgame',
             '^mythbuntu/mythmovies',
             '^mythbuntu/mythmusic',
             '^mythbuntu/mythnews',
             '^mythbuntu/mythphone',
             '^mythbuntu/mythstream',
             '^mythbuntu/mythvideo',
             '^mythbuntu/mythweather',
             '^mythbuntu/mythweb']
        return (['/usr/share/ubiquity/ask-plugins'], questions)

    def run(self,priority,question):
        if question.startswith('mythbuntu/mytharchive'):
            mytharchive = self.frontend.get_mytharchive()
            self.preseed_bool('mythbuntu/mytharchive', mytharchive)
        elif question.startswith('mythbuntu/mythbrowser'):
            mythbrowser = self.frontend.get_mythbrowser()
            self.preseed_bool('mythbuntu/mythbrowser', mythbrowser)
        elif question.startswith('mythbuntu/mythcontrols'):
            mythcontrols = self.frontend.get_mythcontrols()
            self.preseed_bool('mythbuntu/mythcontrols', mythcontrols)
        elif question.startswith('mythbuntu/mythflix'):
            mythflix = self.frontend.get_mythflix()
            self.preseed_bool('mythbuntu/mythflix', mythflix)
        elif question.startswith('mythbuntu/mythgallery'):
            mythgallery = self.frontend.get_mythgallery()
            self.preseed_bool('mythbuntu/mythgallery', mythgallery)
        elif question.startswith('mythbuntu/mythgame'):
            mythgame = self.frontend.get_mythgame()
            self.preseed_bool('mythbuntu/mythgame', mythgame)
        elif question.startswith('mythbuntu/mythmovies'):
            mythmovies = self.frontend.get_mythmovies()
            self.preseed_bool('mythbuntu/mythmovies', mythmovies)
        elif question.startswith('mythbuntu/mythmusic'):
            mythmusic = self.frontend.get_mythmusic()
            self.preseed_bool('mythbuntu/mythmusic', mythmusic)
        elif question.startswith('mythbuntu/mythnews'):
            mythnews = self.frontend.get_mythnews()
            self.preseed_bool('mythbuntu/mythnews', mythnews)
        elif question.startswith('mythbuntu/mythphone'):
            mythphone = self.frontend.get_mythphone()
            self.preseed_bool('mythbuntu/mythphone', mythphone)
        elif question.startswith('mythbuntu/mythphone'):
            mythstream = self.frontend.get_mythstream()
            self.preseed_bool('mythbuntu/mythstream', mythstream)
        elif question.startswith('mythbuntu/mythvideo'):
            mythvideo = self.frontend.get_mythvideo()
            self.preseed_bool('mythbuntu/mythvideo', mythvideo)
        elif question.startswith('mythbuntu/mythweather'):
            mythweather = self.frontend.get_mythweather()
            self.preseed_bool('mythbuntu/mythweather', mythweather)
        elif question.startswith('mythbuntu/mythweb'):
            mythweb = self.frontend.get_mythweb()
            self.preseed_bool('mythbuntu/mythweb', mythweb)
        return FilteredCommand.run(self, priority, question)

    def ok_handler(self):
        mytharchive = self.frontend.get_mytharchive()
        self.preseed_bool('mythbuntu/mytharchive', mytharchive)
        mythbrowser = self.frontend.get_mythbrowser()
        self.preseed_bool('mythbuntu/mythbrowser', mythbrowser)
        mythcontrols = self.frontend.get_mythcontrols()
        self.preseed_bool('mythbuntu/mythcontrols', mythcontrols)
        mythflix = self.frontend.get_mythflix()
        self.preseed_bool('mythbuntu/mythflix', mythflix)
        mythgallery = self.frontend.get_mythgallery()
        self.preseed_bool('mythbuntu/mythgallery', mythgallery)
        mythgame = self.frontend.get_mythgame()
        self.preseed_bool('mythbuntu/mythgame', mythgame)
        mythmovies = self.frontend.get_mythmovies()
        self.preseed_bool('mythbuntu/mythmovies', mythmovies)
        mythmusic = self.frontend.get_mythmusic()
        self.preseed_bool('mythbuntu/mythmusic', mythmusic)
        mythnews = self.frontend.get_mythnews()
        self.preseed_bool('mythbuntu/mythnews', mythnews)
        mythphone = self.frontend.get_mythphone()
        self.preseed_bool('mythbuntu/mythphone', mythphone)
        mythstream = self.frontend.get_mythstream()
        self.preseed_bool('mythbuntu/mythstream', mythstream)
        mythvideo = self.frontend.get_mythvideo()
        self.preseed_bool('mythbuntu/mythvideo', mythvideo)
        mythweather = self.frontend.get_mythweather()
        self.preseed_bool('mythbuntu/mythweather', mythweather)
        mythweb = self.frontend.get_mythweb()
        self.preseed_bool('mythbuntu/mythweb', mythweb)
        FilteredCommand.ok_handler(self)

class MythbuntuThemes(FilteredCommand):
#since all themes are pre-installed, we are seeding the ones
#that will be *removed*
    def prepare(self):
        questions = ['^mythbuntu/officialthemes',
             '^mythbuntu/communitythemes']
        return (['/usr/share/ubiquity/ask-themes'], questions)

    def run(self,priority,question):
        if question.startswith('mythbuntu/officialthemes'):
            official = self.frontend.get_officialthemes()
            official_string=""
            for theme in official:
                if not official[theme].get_active():
                    official_string+=theme + " "
            self.preseed('mythbuntu/officialthemes', official_string)
        elif question.startswith('mythbuntu/communitythemes'):
            community = self.frontend.get_communitythemes()
            community_string=""
            for theme in community:
                if not community[theme].get_active():
                    community_string+=theme + " "
            self.preseed('mythbuntu/communitythemes', community_string)

        return FilteredCommand.run(self, priority, question)

    def ok_handler(self):
        official = self.frontend.get_officialthemes()
        official_string=""
        for theme in official:
            if not official[theme].get_active():
                official_string+=theme + " "
        self.preseed('mythbuntu/officialthemes', official_string)
        community = self.frontend.get_communitythemes()
        community_string=""
        for theme in community:
            if not community[theme].get_active():
                community_string+=theme + " "
        self.preseed('mythbuntu/communitythemes', community_string)
        FilteredCommand.ok_handler(self)

class MythbuntuServices(FilteredCommand):
    def prepare(self):
        questions = [
             '^mythbuntu/vncservice',
             '^mythbuntu/vnc_password',
             '^mythbuntu/sshservice',
             '^mythbuntu/sambaservice',
             '^mythbuntu/nfsservice',
             '^mythbuntu/mysqlservice']
        return (['/usr/share/ubiquity/ask-services'], questions)

    def run(self,priority,question):
        if question.startswith('mythbuntu/vncservice'):
            vnc = self.frontend.get_vnc()
            self.preseed_bool('mythbuntu/vncservice', vnc)
        elif question.startswith('mythbuntu/vnc_password'):
            if not self.frontend.get_vnc():
                vnc_pass = "N/A"
            else:
                vnc_pass = self.frontend.get_vnc_password()
            self.preseed('mythbuntu/vnc_password', vnc_pass)
        elif question.startswith('mythbuntu/sshservice'):
            ssh = self.frontend.get_ssh()
            self.preseed_bool('mythbuntu/sshservice', ssh)
        elif question.startswith('mythbuntu/sambaservice'):
            samba = self.frontend.get_samba()
            self.preseed_bool('mythbuntu/sambaservice', samba)
        elif question.startswith('mythbuntu/nfsservice'):
            nfs = self.frontend.get_nfs()
            self.preseed_bool('mythbuntu/nfsservice', nfs)
        elif question.startswith('mythbuntu/mysqlservice'):
            mysql_secure = self.frontend.get_mysql_port()
            self.preseed_bool('mythbuntu/mysqlservice', mysql_secure)

        return FilteredCommand.run(self, priority, question)

    def ok_handler(self):
        vnc = self.frontend.get_vnc()
        self.preseed_bool('mythbuntu/vncservice', vnc)
        if not self.frontend.get_vnc():
            vnc_pass = "N/A"
        else:
            vnc_pass = self.frontend.get_vnc_password()
        self.preseed('mythbuntu/vnc_password', vnc_pass)
        ssh = self.frontend.get_ssh()
        self.preseed_bool('mythbuntu/sshservice', ssh)
        samba = self.frontend.get_samba()
        self.preseed_bool('mythbuntu/sambaservice', samba)
        nfs = self.frontend.get_nfs()
        self.preseed_bool('mythbuntu/nfsservice', nfs)
        mysql_secure = self.frontend.get_mysql_port()
        self.preseed_bool('mythbuntu/mysqlservice', mysql_secure)
        FilteredCommand.ok_handler(self)

class MythbuntuPasswords(FilteredCommand):
    def prepare(self):
        questions = ['^mythtv/mysql_admin_password',
             '^mythtv/mysql_mythtv_user',
             '^mythtv/mysql_mythtv_password',
             '^mythtv/mysql_mythtv_dbname',
             '^mythtv/mysql_host',
             '^mythweb/enable',
             '^mythweb/username',
             '^mythweb/password']
        return (['/usr/share/ubiquity/ask-passwords'], questions)

    def run(self,priority,question):
        if question.startswith('mythtv/mysql_admin_password'):
            if self.frontend.get_secure_mysql():
                mysql_root = self.frontend.get_mysql_root_password()
            else:
                mysql_root = ""
            self.preseed('mythtv/mysql_admin_password',mysql_root)
        elif question.startswith('mythtv/mysql_mythtv_user'):
            if not self.frontend.get_uselivemysqlinfo():
                mysqluser = self.frontend.get_mysqluser()
            else:
                mysqluser = self.db.get('mythtv/mysql_mythtv_user')
            self.preseed('mythtv/mysql_mythtv_user', mysqluser)
        elif question.startswith('mythtv/mysql_mythtv_password'):
            if not self.frontend.get_uselivemysqlinfo():
                mysqlpass = self.frontend.get_mysqlpass()
            else:
                mysqlpass = self.db.get('mythtv/mysql_mythtv_password')
            self.preseed('mythtv/mysql_mythtv_password', mysqlpass)
        elif question.startswith('mythtv/mysql_mythtv_dbname'):
            if not self.frontend.get_uselivemysqlinfo():
                mysqldatabase = self.frontend.get_mysqldatabase()
            else:
                mysqldatabase = self.db.get('mythtv/mysql_mythtv_dbname')
            self.preseed('mythtv/mysql_mythtv_dbname', mysqldatabase)
        elif question.startswith('mythtv/mysql_host'):
            if not self.frontend.get_uselivemysqlinfo():
                mysqlserver = self.frontend.get_mysqlserver()
            else:
                mysqlserver = self.db.get('mythtv/mysql_host')
            self.preseed('mythtv/mysql_host', mysqlserver)
        elif question.startswith('mythweb/enable'):
            auth = self.frontend.get_secure_mythweb()
            self.preseed_bool('mythweb/enable', auth)
        elif question.startswith('mythweb/username'):
            user = self.frontend.get_mythweb_username()
            self.preseed('mythweb/username', user)
        elif question.startswith('mythweb/password'):
            passw = self.frontend.get_mythweb_password()
            self.preseed('mythweb/password', passw)
        return FilteredCommand.run(self, priority, question)

def ok_handler(self):
        if self.frontend.get_secure_mysql():
            mysql_root = self.frontend.get_mysql_root_password()
        else:
            mysql_root = ""
        self.preseed('mythtv/mysql_admin_password',mysql_root)
        if not self.frontend.get_uselivemysqlinfo():
            mysqluser = self.frontend.get_mysqluser()
        else:
            mysqluser = self.db.get('mythtv/mysql_mythtv_user')
        self.preseed('mythtv/mysql_mythtv_user', mysqluser)
        if not self.frontend.get_uselivemysqlinfo():
            mysqlpass = self.frontend.get_mysqlpass()
        else:
            mysqlpass = self.db.get('mythtv/mysql_mythtv_password')
        self.preseed('mythtv/mysql_mythtv_password', mysqlpass)
        if not self.frontend.get_uselivemysqlinfo():
            mysqldatabase = self.frontend.get_mysqldatabase()
        else:
            mysqldatabase = self.db.get('mythtv/mysql_mythtv_dbname')
        self.preseed('mythtv/mysql_mythtv_dbname', mysqldatabase)
        if not self.frontend.get_uselivemysqlinfo():
            mysqlserver = self.frontend.get_mysqlserver()
        else:
            mysqlserver = self.db.get('mythtv/mysql_host')
        self.preseed('mythtv/mysql_host', mysqlserver)
        auth = self.frontend.get_secure_mythweb()
        self.preseed_bool('mythweb/enable', auth)
        user = self.frontend.get_mythweb_username()
        self.preseed('mythweb/username', user)
        passw = self.frontend.get_mythweb_password()
        self.preseed('mythweb/password', passw)
        FilteredCommand.ok_handler(self)

class MythbuntuRemote(FilteredCommand):
    def prepare(self):
        questions = ['^lirc/remote',
             '^lirc/remote_lircd_conf',
             '^lirc/remote_modules',
             '^lirc/remote_driver',
             '^lirc/transmitter',
             '^lirc/transmitter_lircd_conf',
             '^lirc/transmitter_modules',
             '^lirc/transmitter_driver']
        return (['/usr/share/ubiquity/ask-ir'], questions)

    def run(self,priority,question):
        if question.startswith('lirc/remote'):
            device=self.frontend.get_lirc("remote")
            if question.startswith('lirc/remote_modules'):
                self.preseed('lirc/remote_modules',device["modules"])
            elif question.startswith('lirc/remote_lircd_conf'):
                self.preseed('lirc/remote_lircd_conf',device["lircd_conf"])
            elif question.startswith('lirc/remote_driver'):
                self.preseed('lirc/remote_driver',device["driver"])
            elif question.startswith('lirc/remote_device'):
                self.preseed('lirc/remote_device',device["device"])
            elif question.startswith('lirc/remote'):
                self.preseed('lirc/remote',device["remote"])
        elif question.startswith('lirc/transmitter'):
            device=self.frontend.get_lirc("transmitter")
            if question.startswith('lirc/transmitter_modules'):
                self.preseed('lirc/transmitter_modules',device["modules"])
            elif question.startswith('lirc/transmitter_lircd_conf'):
                self.preseed('lirc/transmitter_lircd_conf',device["lircd_conf"])
            elif question.startswith('lirc/transmitter_driver'):
                self.preseed('lirc/transmitter_driver',device["driver"])
            elif question.startswith('lirc/transmitter_device'):
                self.preseed('lirc/transmitter_device',device["device"])
            elif question.startswith('lirc/transmitter'):
                self.preseed('lirc/transmitter',device["transmitter"])
        return FilteredCommand.run(self, priority, question)

    def ok_handler(self):
        device = self.frontend.get_lirc("remote")
        self.preseed('lirc/remote_modules',device["modules"])
        self.preseed('lirc/remote_lircd_conf',device["lircd_conf"])
        self.preseed('lirc/remote_driver',device["driver"])
        self.preseed('lirc/remote_device',device["device"])
        self.preseed('lirc/remote',device["remote"])
        device = self.frontend.get_lirc("transmitter")
        self.preseed('lirc/transmitter_modules',device["modules"])
        self.preseed('lirc/transmitter_lircd_conf',device["lircd_conf"])
        self.preseed('lirc/transmitter_driver',device["driver"])
        self.preseed('lirc/transmitter_device',device["device"])
        self.preseed('lirc/transmitter',device["transmitter"])
        FilteredCommand.ok_handler(self)

class MythbuntuDrivers(FilteredCommand):
    def prepare(self):
        questions = ['^mythbuntu/video_driver',
             '^mythbuntu/tvout',
             '^mythbuntu/tvstandard',
             '^mythbuntu/hdhomerun',
             '^mythbuntu/xmltv',
             '^mythbuntu/dvbutils']
        return (['/usr/share/ubiquity/ask-drivers'], questions)

    def run(self,priority,question):
        if question.startswith('mythbuntu/video_driver'):
            video_driver = self.frontend.get_video()
            self.preseed('mythbuntu/video_driver', video_driver)
        elif question.startswith('mythbuntu/tvout'):
            tvout = self.frontend.get_tvout()
            self.preseed('mythbuntu/tvout', tvout)
        elif question.startswith('mythbuntu/tvstandard'):
            tvstandard = self.frontend.get_tvstandard()
            self.preseed('mythbuntu/tvstandard', tvstandard)
        elif question.startswith('mythbuntu/hdhomerun'):
            hdhomerun = self.frontend.get_hdhomerun()
            self.preseed_bool('mythbuntu/hdhomerun',hdhomerun)
        elif question.startswith('mythbuntu/xmltv'):
            xmltv = self.frontend.get_xmltv()
            self.preseed_bool('mythbuntu/xmltv',xmltv)
        elif question.startswith('mythbuntu/dvbutils'):
            dvbutils = self.frontend.get_dvbutils()
            self.preseed_bool('mythbuntu/dvbutils',dvbutils)
        return FilteredCommand.run(self, priority, question)

    def ok_handler(self):
        video_driver = self.frontend.get_video()
        self.preseed('mythbuntu/video_driver', video_driver)
        tvout = self.frontend.get_tvout()
        self.preseed('mythbuntu/tvout', tvout)
        tvstandard = self.frontend.get_tvstandard()
        self.preseed('mythbuntu/tvstandard', tvstandard)
        hdhomerun = self.frontend.get_hdhomerun()
        self.preseed_bool('mythbuntu/hdhomerun',hdhomerun)
        FilteredCommand.ok_handler(self)
