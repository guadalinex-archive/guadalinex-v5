# -*- coding: utf-8 -*-

import sys
sys.path.insert(0, './')

import gtk
import gtk.glade
import gettext

from hermes_hardware import setup_gettext
from devicelist import DeviceList


class Controller(object):

    def __init__(self):
        xml = gtk.glade.XML('coldassistant/ui.glade', root='mainwindow')
        xml.signal_autoconnect(self)

        self.window = xml.get_widget('mainwindow')
        self.finalize = False

        icon = gtk.gdk.pixbuf_new_from_file("/usr/share/hermes/img/logo.svg")
        self.window.set_icon(icon)
        
        vbox = xml.get_widget('treeview_vbox')

        devicelist = DeviceList()
        vbox.pack_start(devicelist, False, False)
        self.devicelist = devicelist


    def reset(self):
        self.devicelist.reset()


    def on_close_clicked(self, widget):
        self.finalize = True
        gtk.main_quit()



if __name__ == '__main__':
    try:
        import defs
        setup_gettext('hermes-hardware', defs.DATA_DIR)
    except ImportError:
        print 'WARNING: Running uninstalled, no gettext support'

    controller = Controller()
    while not controller.finalize:
        controller.reset()
        gtk.main()
