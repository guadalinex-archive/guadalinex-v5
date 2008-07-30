#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Authors : Cesar Garcia Tapia <tapia@openshine.com> 
#           Roberto Majadas <roberto.majadas@openshine.com>
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


import gtk
import gtk.glade
import gtkmozembed
import gobject
import MSD
import feedparser
import time
import pickle
import subprocess
from MSDUtils import *

class MSDRSSFeeder:
    def __init__ (self):
        # FILES AND URLS
        self.rss_file = os.path.join(os.environ["HOME"], ".movistar_desktop/", "rss.xml")
        self.cache_file = os.path.join(os.environ["HOME"], ".movistar_desktop/", "cache")
#       self.url = self.conf.get_updater_feed_url()
#        self.url = 'http://195.235.93.150/rss_EM_Linux.xml'
        self.url = 'http://www.movistar.es/empresas/servicios/descargaaplicaciones/linux/rss.xml'

	self.ver_leidos = True
	self.ver_no_leidos = True
	self.feed = None
	
	self.xml = gtk.glade.XML(MSD.glade_files_dir + "rssfeeder.glade")
	self.window = self.xml.get_widget("rss_window")
        self.window.set_icon_from_file(MSD.icons_files_dir + "main_window_icon.png")
        self.window.hide()
	self.noticias_treeview = self.xml.get_widget("noticias_treeview")
	self.texto_scrolledwindow = self.xml.get_widget("texto_scrolledwindow")

	self.abrir_toolbutton = self.xml.get_widget ("abrir_toolbutton")
	self.todos_toolbutton = self.xml.get_widget ("todos_toolbutton")
	self.leidos_toolbutton = self.xml.get_widget ("leidos_toolbutton")
	self.noleidos_toolbutton = self.xml.get_widget ("noleidos_toolbutton")
	
	self.window.connect("delete_event", self.__on_rss_window_delete)
#	self.window.connect("destroy", self.__on_rss_window_destroy)
	
	dict = {"on_abrir_toolbutton_clicked": self.__on_abrir_toolbutton_clicked,
		"on_refrescar_toolbutton_clicked": self.__on_refrescar_toolbutton_clicked,
		"on_noticias_treeview_button_press_event": self.__on_noticias_treeview_button_press_event
                }
        self.xml.signal_autoconnect(dict)

        col = gtk.TreeViewColumn(_(u"Leído"))
        self.noticias_treeview.append_column(col)
        cell = gtk.CellRendererToggle()
        col.pack_start(cell, True)
        col.add_attribute(cell, 'active', 3)
        col.set_resizable(True)
        col.set_reorderable(False)

        col = gtk.TreeViewColumn(_(u"Título"))
        self.noticias_treeview.append_column(col)
        cell = gtk.CellRendererText()
        col.pack_start(cell, True)
        col.add_attribute(cell, 'text', 4)
        col.set_sort_column_id(1)
        col.set_resizable(True)
        col.set_reorderable(False)
        cell.set_property("editable", False)
	
        col = gtk.TreeViewColumn(_(u"Fecha"))
        self.noticias_treeview.append_column(col)
        cell = gtk.CellRendererText()
        col.pack_start(cell, True)
        col.add_attribute(cell, 'text', 5)
        col.set_sort_column_id(1)
        col.set_resizable(True)
        col.set_reorderable(False)
        cell.set_property("editable", False)

	self.noticias_model = gtk.ListStore(gobject.TYPE_INT,
	                                    gobject.TYPE_STRING,
	                                    gobject.TYPE_STRING,
					    gobject.TYPE_BOOLEAN,
	                                    gobject.TYPE_STRING,
					    gobject.TYPE_STRING)
        self.noticias_treeview.set_model(self.noticias_model)
        self.noticias_treeview_selection = self.noticias_treeview.get_selection()
        self.noticias_treeview_selection.connect ("changed", self.__on_noticias_treeview_selection_changed)

        self.viewer = gtkmozembed.MozEmbed()
	self.texto_scrolledwindow.add_with_viewport (self.viewer)
	data = ''
	self.viewer.render_data(data, long(len(data)), 'file:///', 'text/html')
	self.viewer.show()

        self.downloader_pid = None

	self.todos_cb_id = self.todos_toolbutton.connect ("clicked", self.__on_todos_toolbutton_clicked)
	self.leidos_cb_id = False
	self.noleidos_cb_id = False
	self.todos_toolbutton.set_active(True)

    def init_rss (self):    
        self.update_rss ()
	self.__refresh_view()
	return False
        
    def check_on_connect(self):
        if self.update_rss() == True:
            self.__refresh_view ()
            self.window.show()

    def show_rss (self):
        self.load_rss()
        self.__refresh_view()
	self.window.show()

    def load_rss (self):
        if os.path.exists (self.rss_file):
	    self.feed = feedparser.parse (self.rss_file)
	    self.read_cache = self.__load_cache()

    def update_rss (self):
        tmpfile = "/tmp/rss-%i.xml" % int(time.time())

        if os.path.exists("/usr/bin/wget"):
            cmd = ["wget", self.url, "-O", tmpfile, "-t", "1", "--timeout=10"]
        elif os.path.exists("/usr/bin/curl"):
            cmd = ["curl", "-o", tmpfile, self.url, "-m", "10"]
        else:
            return False

        returncode = None
        run = subprocess.Popen(cmd)
	self.downloader_pid = run.pid
        while(returncode is None):
            gobject.main_context_default ().iteration ()
            returncode = run.poll()

        if returncode != 0:
            self.downloader_pid = None
            self.read_cache = self.__load_cache()
            return False

        self.downloader_pid = None
        actualizado = False

        if os.path.exists (self.rss_file):
	    d1 = feedparser.parse (self.rss_file)
	    d2 = feedparser.parse (tmpfile)

            if d2.feed.updated_parsed <= d1.feed.updated_parsed:
                self.feed = d1
		actualizado = False
	    else:
                subprocess.Popen(["mv", tmpfile, self.rss_file])
                self.feed = d2
		actualizado = True
        else:
            subprocess.Popen(["mv", tmpfile, self.rss_file])
            self.feed = feedparser.parse (tmpfile)
	    actualizado = True

        self.read_cache = self.__load_cache()

	return actualizado

    def kill_downloader (self):
	if self.downloader_pid != None:
	    os.kill (self.downloader_pid, 15)

    def __load_treeview (self):
        self.noticias_model.clear()
        if self.feed != None:
            i = 0
            for entry in self.feed.entries:
                if entry.id in self.readed_news:
	            if self.ver_leidos:
		        (pub_year, pub_month, pub_day, pub_hour, pub_minute, pub_second, a, b, c) = entry.updated_parsed
			date = "%d/%d/%d - %d:%d:%d" % (pub_day, pub_month, pub_year, pub_hour, pub_minute, pub_second)
		        self.noticias_model.append ([i, entry.id, entry.link, True,  entry.title, date])
                else:
	            if self.ver_no_leidos:
		        (pub_year, pub_month, pub_day, pub_hour, pub_minute, pub_second, a, b, c) = entry.updated_parsed
			date = "%d/%d/%d - %d:%d:%d" % (pub_day, pub_month, pub_year, pub_hour, pub_minute, pub_second)
                        self.noticias_model.append ([i, entry.id, entry.link, False, entry.title, date])
                i += 1
	
	return False

    def __load_cache (self):
        if os.path.exists(self.cache_file):
            fd = open(self.cache_file, "r")
            self.readed_news = pickle.loads(fd.read())
            fd.close()
        else:
            self.readed_news = []

        new_readed_news = []
        if self.feed != None:
            for item in self.feed.entries:
                if item.id in self.readed_news:
                    new_readed_news.append (item.id)
        self.readed_news = new_readed_news

    def __save_cache (self):
        fd = open(self.cache_file, "w")
        fd.write(pickle.dumps(self.readed_news))
        fd.close()

    def __on_noticias_treeview_selection_changed (self, treeselection):
        if treeselection.count_selected_rows() > 0:
            model, itera = treeselection.get_selected()
            num_elemento = int(self.noticias_model.get_value(itera, 0))
            item_id = self.noticias_model.get_value(itera, 1)
	    self.abrir_toolbutton.set_sensitive (True)
	else:
	    self.abrir_toolbutton.set_sensitive (False)
	    return

        data = self.feed.entries[num_elemento].summary.encode('latin-1')
        self.viewer.render_data(data, long(len(data)), 'file:///', 'text/html')

        if not item_id in self.readed_news:
            self.readed_news.append (item_id)
            self.noticias_model.set_value (itera, 3, True)
            self.__save_cache()
        else:
            self.viewer.render_data(data, long(len(data)), 'file:///', 'text/html')

    def __on_abrir_toolbutton_clicked (self, widget, data=None):
        treeselection = self.noticias_treeview.get_selection()
        if treeselection.count_selected_rows() > 0:
            model, itera = treeselection.get_selected()
            item_link = self.noticias_model.get_value(itera, 2)
            subprocess.Popen(["gnome-open", item_link])

    def __on_noticias_treeview_button_press_event (self, widget, event):
        if event.type == gtk.gdk._2BUTTON_PRESS:
	    self.__on_abrir_toolbutton_clicked(widget)

    def __on_todos_toolbutton_clicked (self, widget, data=None):
    	if self.todos_cb_id != False:
	    self.todos_toolbutton.disconnect (self.todos_cb_id)
    	if self.leidos_cb_id != False:
	    self.leidos_toolbutton.disconnect (self.leidos_cb_id)
    	if self.noleidos_cb_id != False:
	    self.noleidos_toolbutton.disconnect (self.noleidos_cb_id)

	self.ver_leidos = True
	self.ver_no_leidos = True
	self.todos_toolbutton.set_active (True)
	self.leidos_toolbutton.set_active (False)
	self.noleidos_toolbutton.set_active (False)
	gobject.timeout_add (1, self.__load_treeview)

	self.todos_cb_id = self.todos_toolbutton.connect ("clicked", self.__on_todos_toolbutton_clicked)
	self.leidos_cb_id = self.leidos_toolbutton.connect ("clicked", self.__on_leidos_toolbutton_clicked)
	self.noleidos_cb_id = self.noleidos_toolbutton.connect ("clicked", self.__on_noleidos_toolbutton_clicked)

    def __on_leidos_toolbutton_clicked (self, widget, data=None):
    	if self.todos_cb_id != False:
	    self.todos_toolbutton.disconnect (self.todos_cb_id)
    	if self.leidos_cb_id != False:
	    self.leidos_toolbutton.disconnect (self.leidos_cb_id)
    	if self.noleidos_cb_id != False:
	    self.noleidos_toolbutton.disconnect (self.noleidos_cb_id)

    	self.ver_leidos = True
	self.ver_no_leidos = False 
	self.todos_toolbutton.set_active (False)
	self.leidos_toolbutton.set_active (True)
	self.noleidos_toolbutton.set_active (False)
	gobject.timeout_add (1, self.__load_treeview)

	self.todos_cb_id = self.todos_toolbutton.connect ("clicked", self.__on_todos_toolbutton_clicked)
	self.leidos_cb_id = self.leidos_toolbutton.connect ("clicked", self.__on_leidos_toolbutton_clicked)
	self.noleidos_cb_id = self.noleidos_toolbutton.connect ("clicked", self.__on_noleidos_toolbutton_clicked)

    def __on_noleidos_toolbutton_clicked (self, widget, data=None):
    	if self.todos_cb_id != False:
	    self.todos_toolbutton.disconnect (self.todos_cb_id)
    	if self.leidos_cb_id != False:
	    self.leidos_toolbutton.disconnect (self.leidos_cb_id)
    	if self.noleidos_cb_id != False:
	    self.noleidos_toolbutton.disconnect (self.noleidos_cb_id)

	self.ver_leidos = False
	self.ver_no_leidos = True
	self.todos_toolbutton.set_active (False)
	self.leidos_toolbutton.set_active (False)
	self.noleidos_toolbutton.set_active (True)
	gobject.timeout_add (1, self.__load_treeview)

	self.todos_cb_id = self.todos_toolbutton.connect ("clicked", self.__on_todos_toolbutton_clicked)
	self.leidos_cb_id = self.leidos_toolbutton.connect ("clicked", self.__on_leidos_toolbutton_clicked)
	self.noleidos_cb_id = self.noleidos_toolbutton.connect ("clicked", self.__on_noleidos_toolbutton_clicked)

    def __on_refrescar_toolbutton_clicked (self, widget, data=None):
        gobject.timeout_add (1, self.init_rss)
	self.__refresh_view ()

    def __refresh_view (self):
	if self.todos_toolbutton.get_active():
	    self.__on_todos_toolbutton_clicked(self.todos_toolbutton)
	elif self.leidos_toolbutton.get_active():
	    self.__on_leidos_toolbutton_clicked(self.leidos_toolbutton)
	elif self.noleidos_toolbutton.get_active():
	    self.__on_noleidos_toolbutton_clicked(self.noleidos_toolbutton)


    def __on_rss_window_delete(self, widget, event, data=None):
        self.window.hide()
        return True


    def main (self):
        gtk.main()

if __name__ == "__main__":
    rss = MSDRSSFeeder()
    rss.show_rss()
    rss.main()
