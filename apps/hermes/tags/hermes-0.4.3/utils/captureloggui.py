#!/usr/bin/python
# -*- coding: utf-8 -*-

import gtk
import gtk.glade
import sys
import tempfile
import logging
import os.path
import webbrowser

class CaptureLogGui(object):

    def __init__(self):
        self.logger = logging.getLogger()

        #Set logfile
        self.__set_logfile()

        #Set interface
        xml = gtk.glade.XML('utils/captureloggui.glade', root = 'mainwindow')
        xml.signal_autoconnect(self)

        self.mainwindow = xml.get_widget('mainwindow')
        icon = gtk.gdk.pixbuf_new_from_file("/usr/share/hermes/img/logo.svg")

        self.mainwindow.set_icon(icon)
        self.mainwindow.show_all()
        self.devnameentry = xml.get_widget('devname')



    # SIGNAL HANDLERS ##################
    def on_capturelog(self, *args):
        self.logfile.close()

        devname = self.devnameentry.get_text() or 'newdevice'

        content = open(self.logfilepath).read()
        newfile = open(os.path.expanduser('~') + \
                '/Desktop/' + devname + '.log', 
                'w')

        newfile.write(content)
        newfile.close()

        self.__set_logfile()
        

    def on_cancel(self, *args):
        gtk.main_quit()


    def on_finish(self, *args):
        gtk.main_quit()


    def on_help_clicked(self, *args):
        webbrowser.open("/usr/share/hermes/doc/html/index.html")


    # Private methods ####################
    def __set_logfile(self):
        self.logfilepath = tempfile.mktemp()

        self.logfile = open(self.logfilepath, 'w')
        sys.stdout = self.logfile





