#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica Móviles España S.A.U.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import os
import gtk
import gobject
import MSD
import md5
import dbus
import dbus.glib
import feedparser
import time
import popen2
from MSD.MSDPPPvariables import *

class MSDUpdateChecker:
    def __init__(self, conf, mcontroller):
        self.conf = conf
        self.mcontroller = mcontroller
        self.process = None

        # Interface
        self.xml = gtk.glade.XML(MSD.glade_files_dir + "updater.glade")
        self.uw_dialog = self.xml.get_widget("u_warning_dialog")
        self.uw_label = self.xml.get_widget("uw_label")
        self.uw_dialog.hide()

        # DBus for connection signals
        self.__connect_to_bus()
        # For Debug pourpose
        #self.check_now(None)
        
        #gobject.timeout_add(5000, self.__real_check_now_cb)
        

    def __connect_to_bus(self):
        #FIXED : Dialer
        if self.mcontroller.dialer != None :
            self.ppp_manager = self.mcontroller.dialer
            #self.ppp_manager.connect("connected", self.check_now)

    def check_now(self, dialer):
        tmpfile = "/tmp/rss-%i.xml" % int(time.time())
        rss_file = os.path.join(os.environ["HOME"], ".movistar_desktop/", "rss.xml")
        url = self.conf.get_updater_feed_url()
        
        if os.path.exists("/usr/bin/wget"):
            cmd = "/usr/bin/wget %s -O %s && mv %s %s " % (url, tmpfile, tmpfile, rss_file)
        elif os.path.exists("/usr/bin/curl"):
            cmd = "/usr/bin/curl -o %s %s && mv %s %s " % (tmpfile, url, tmpfile, rss_file)
        else:
            return
        
        self.process = popen2.Popen3(cmd)

    def __real_check_now_cb(self):
        rss_file = os.path.join(os.environ["HOME"], ".movistar_desktop/", "rss.xml")
        
        if not os.path.exists(rss_file) :
            return True
        
        d = feedparser.parse(rss_file)

        os.system("rm %s" % rss_file)
        
        if (len(d['entries']) < 1):
            print _(u"No entries in RSS")
            return True
        
        # Check date with saved feed
        new_feed_date = md5.new(d.entries[0].date).hexdigest()
        saved_feed_date = md5.new(self.conf.get_updater_feed_date()).hexdigest()
        release_date_parsed = feedparser._parse_date(self.conf.get_release_date())
        # Debug
        print "RSS-----------------"
        print d.entries[0]
        print "--------------------"
        print "new_feed_date %s" % new_feed_date
        print "saved_feed_date %s" % saved_feed_date
        print "fecha: %s" % d.entries[0].updated_parsed
        print "release: %s" % release_date_parsed
        # End debug
        
        if ((new_feed_date != saved_feed_date) and (release_date_parsed < d.entries[0].updated_parsed)):
            self.uw_dialog.set_title(d.entries[0].title)
            self.uw_label.set_text(d.entries[0].description)

            self.uw_dialog.show_all()
            result = self.uw_dialog.run()
            if (result == gtk.RESPONSE_OK):
                os.system("gnome-open %s" % d.entries[0].link)
                self.conf.set_updater_feed_date(d.entries[0].date)
                self.conf.save_conf()
            elif (result == gtk.RESPONSE_NO):
                self.conf.set_updater_feed_date(d.entries[0].date)
                self.conf.save_conf()

            self.uw_dialog.hide()

        return True
