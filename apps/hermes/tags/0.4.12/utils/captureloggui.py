# -*- coding: utf-8 -*-

import gtk
import gtk.glade
import sys
import tempfile
import logging
import os.path
import webbrowser

def get_user_dir(key):
    """
    Obtiene las rutas completas de los directorios xdg-user-dirs.
    http://www.freedesktop.org/wiki/Software/xdg-user-dirs
        XDG_DESKTOP_DIR
        XDG_DOWNLOAD_DIR
        XDG_TEMPLATES_DIR
        XDG_PUBLICSHARE_DIR
        XDG_DOCUMENTS_DIR
        XDG_MUSIC_DIR
        XDG_PICTURES_DIR
        XDG_VIDEOS_DIR
    """
    user_dirs_dirs = os.path.expanduser("~/.config/user-dirs.dirs")
    if os.path.exists(user_dirs_dirs):
        f = open(user_dirs_dirs, "r")
        for line in f.readlines():
            if line.startswith(key):
                return os.path.expandvars(line[len(key)+2:-2])
    return False

xdg_desktop_dir = get_user_dir("XDG_DESKTOP_DIR")
if xdg_desktop_dir and os.path.exists(xdg_desktop_dir):
    desktop_dir = xdg_desktop_dir
else:
    desktop_dir=os.environ['HOME']


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
        newfile = open(desktop_dir + os.sep + devname + '.log', 'w')

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
