#!/usr/bin/env python
# coding: utf-8
#
# Copyright (C) 2004-2005 Ross Burton <ross@burtonini.com>
#               2005-2006 Canonical
#               2006 Sebastian Heinlein
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA 02111-1307 USA

# Don't forget to disbale this :)
#import pdb
import pygtk; pygtk.require("2.0")
import gtk
import gtk.glade
import gtk.gdk
import gobject
import gconf
import pango

import gettext
from gettext import gettext as _

#setup gettext
app="gnome-app-install"
gettext.textdomain(app)
gettext.bindtextdomain(app)
gtk.glade.textdomain(app)
gtk.glade.bindtextdomain(app)


import stat
import glob
import re
import subprocess
import tempfile
import warnings
import os
import sys
from datetime import datetime

import dbus
import dbus.service
import dbus.glib

from warnings import warn
warnings.filterwarnings("ignore", "ICON:.*", UserWarning)
warnings.filterwarnings("ignore", "apt API not stable yet", FutureWarning)
import apt
import apt_pkg

# from update-manager, needs to be factored out
from UpdateManager.Common.aptsources import SourcesList, is_mirror

# internal imports
from AppInstall.DialogNewlyInstalled import DialogNewlyInstalled
from AppInstall.DialogPendingChanges import DialogPendingChanges
from AppInstall.DialogMultipleApps import DialogMultipleApps
from AppInstall.DialogUnavailable import DialogUnavailable
from AppInstall.DialogProprietary import DialogProprietary
from AppInstall.PackageWorker import PackageWorker
from AppInstall.Menu import ApplicationMenu

# FIXME: Share this with update-manager
#        the version in g-a-i is a little bit ahead :)
from AppInstall.ReleaseNotesViewer import ReleaseNotesViewer
from AppInstall.SimpleGladeApp import SimpleGladeApp
from AppInstall.Progress import GtkOpProgressWindow
from AppInstall.Util import *
import AppInstall.common as common


# this is used to guess the desktop environment that the application
# was written for
desktop_environment_mapping = {
    ("kdelibs4c2a","python-kde3","libqt3-mt") :
    (_("%s integrates well into the Kubuntu desktop"), "application-kubuntu"),
    ("libgnome2-0","python-gnome2","libgtk2.0-0","python-gtk2") :
    (_("%s integrates well into the Ubuntu desktop"), "application-ubuntu"),
    ("libgnustep-base1.11") :
    (_("%s integrates well into the Gnustep desktop"), None),
    ("libxfce4util4",) :
    (_("%s integrates well into the Xubuntu desktop"), None),
}

from AppInstall.Menu import SHOW_ALL, SHOW_ONLY_SUPPORTED, SHOW_ONLY_FREE, SHOW_ONLY_MAIN, SHOW_ONLY_PROPRIETARY, SHOW_ONLY_THIRD_PARTY, SHOW_ONLY_INSTALLED



class AppInstallDbusControler(dbus.service.Object):
    """ this is a helper to provide the AppInstallIFace """
    def __init__(self, parent, bus_name,
                 object_path='/org/freedesktop/AppInstallObject'):
        dbus.service.Object.__init__(self, bus_name, object_path)
        self.parent = parent

    @dbus.service.method('org.freedesktop.AppInstallIFace')
    def bringToFront(self):
        self.parent.window_main.present()
        return True
    
class AppInstall(SimpleGladeApp):

    def __init__(self, datadir, desktopdir, cachedir, arguments=None, 
                 mime_search=None):
        self.setupDbus()

        self.search_timeout_id = 0

        # setup a default icon
        self.icons = common.ToughIconTheme()
        gtk.window_set_default_icon(self.icons.load_icon("gnome-app-install", 32, 0))

        SimpleGladeApp.__init__(self, domain="gnome-app-install",
                                path=datadir+"/gnome-app-install.glade")

        self.channelsdir = desktopdir+"/channels"       
        self.datadir = datadir
        self.cachedir = cachedir
        self.desktopdir = desktopdir

        # sensitive stuff
        self.button_apply.set_sensitive(False)

        # create the treeview
        self.setupTreeview()

        # are we sorting by popcon
        self.sort_by_ranking = False

        # setup the gconf backend
        self.config = gconf.client_get_default()
        self.config.add_dir ("/apps/gnome-app-install", gconf.CLIENT_PRELOAD_NONE)

        # Tooltips
        self.tooltips = gtk.Tooltips()
        self.tipmap = {}

        # combobx with filters for the application list
        filter_to_restore = self.config.get_int("/apps/gnome-app-install/filter_applications")
        if filter_to_restore not in range(5):
            filter_to_restore = 0
        list_filters = gtk.ListStore(gobject.TYPE_STRING,
                                     gobject.TYPE_BOOLEAN,
                                     gobject.TYPE_STRING,
                                     gobject.TYPE_INT)
        self.combobox_filter.set_model(list_filters)
        filter_renderer = gtk.CellRendererText()
        self.combobox_filter.pack_start(filter_renderer)
        for (desc, sep, tooltip, filter) in [
            (_("All available applications"), False,
             _("Show all applications including ones which are possibly "
               "restricted by law or copyright, "
               "unsupported by Canonical Ltd. or not part of Ubuntu"),
             SHOW_ALL),
            (_("All Open Source applications"), False,
             _("Show all Ubuntu applications which can be freely used, "
               "modified and distributed. This includes a large "
               "variety of community maintained applications"),
             SHOW_ONLY_FREE),
            ("separator", True, "separator", -1),
            (_("Supported Ubuntu applications"), False,
             _("Show only Ubuntu applications which come with full "
               "technical and security support by Canonical Ltd. "
               "All applications can be "
               "freely used, modified and distributed"),
             SHOW_ONLY_MAIN),
            (_("Third party applications"), False,
             _("Show only applications that are provided by "
               "independent software vendors and are not part of Ubuntu"), 
             SHOW_ONLY_THIRD_PARTY),
            ("separator", True, "separator", -1),
            (_("Installed applications"), False, _("Show only applications "\
               "that are installed on your computer"), SHOW_ONLY_INSTALLED)
            ]:
            list_filters.append((desc, sep, tooltip, filter))
            self.combobox_filter.set_row_separator_func(self.separator_filter)
            self.combobox_filter.set_cell_data_func(filter_renderer, 
                                                    self.tooltip_on_filter)
            if filter == filter_to_restore:
                self.combobox_filter.set_active(len(list_filters) - 1)
                self.tooltips.set_tip(self.eventbox_filter, tooltip)

        # connect the changed signal of the combobox
        self.combobox_filter.connect("changed", self.on_combobox_filter_changed)

        self.textview_description = ReleaseNotesViewer()
        self.textview_description.set_wrap_mode(gtk.WRAP_WORD)
        #self.textview_description.set_pixels_above_lines(6)
        self.textview_description.set_pixels_below_lines(3)
        self.textview_description.set_right_margin(6)
        self.textview_description.set_left_margin(6)
        self.scrolled_description.add(self.textview_description)
        self.scrolled_description.set_policy(gtk.POLICY_AUTOMATIC, 
                                             gtk.POLICY_AUTOMATIC)
        atk_desc = self.textview_description.get_accessible()
        atk_desc.set_name(_("Description"))

        # Add a fake liststore to the packages list, so that the headers
        # are already seen during start up
        fake_applications = gtk.ListStore(gobject.TYPE_INT,
                                          gobject.TYPE_STRING,
                                          gobject.TYPE_PYOBJECT,
                                          gobject.TYPE_INT)
        fake_applications.set_sort_column_id(COL_NAME, gtk.SORT_ASCENDING)
        self.treeview_packages.set_model(fake_applications)

        # now show the main window ...
        if mime_search:
            self.label_progress.set_markup("<big><b>%s</b></big>\n\n%s" %
                                         (_("Searching for appropriate "
                                            "applications"),
                                          _("Please wait. This might take a "
                                            "minute or two.")))
        else:
            self.window_main.show()

        # ... and open the cache
        self.updateCache(filter_to_restore)

        self.search_entry.grab_focus()

        self.show_intro()
        self.textview_description.show()

        # this is a set() of packagenames that contain multiple applications
        # if a pkgname is in the set, a dialog was already displayed to the
        # user about this (ugly ...)
        self.multiple_pkgs_seen = set()

        # handle arguments
	if mime_search:
            self.window_main.show()
            self.menu.mimeSearch = mime_search
	    self.scrolledwindow_left.hide()
            self.search_entry.set_text(mime_search.string)
            self.on_search_timeout()
                    
        # create a worker that does the actual installing etc
        self.packageWorker = PackageWorker()

        # used for restore_state
        self.last_toggle = None

        # now check if the cache is up-to-date
        time_cache = 0
        for f in glob.glob("/var/lib/apt/lists/*Packages"):
            mt = os.stat(f)[stat.ST_MTIME]
            ct = os.stat(f)[stat.ST_CTIME] 
            if mt > time_cache:
               time_cache = mt
            if ct > time_cache:
               time_cache = ct
        time_source = os.stat("/etc/apt/sources.list")[stat.ST_MTIME]
        for f in glob.glob("/etc/apt/sources.list.d/*.list"):
            mt = os.stat(f)[stat.ST_MTIME]
            ct = os.stat(f)[stat.ST_CTIME]
            if mt > time_source:
                time_source = mt
            if ct > time_source:
                time_source = ct
        # FIXME: problem: what can happen is that the sources.list is modified
        #        but we only get I-M-S hits and the mtime of the Packages
        #        files do not change
        #print "cache:  ", time_cache
        #print "source: ", time_source
        if time_cache < time_source:
            self.dialog_cache_outdated.set_transient_for(self.window_main)
            self.dialog_cache_outdated.realize()
            self.dialog_cache_outdated.window.set_functions(gtk.gdk.FUNC_MOVE)
            res = self.dialog_cache_outdated.run()
            self.dialog_cache_outdated.hide()
            if res == gtk.RESPONSE_YES:
                self.reloadSources()

    def separator_filter(self, model, iter, user_data=None):
        """Used to draw a spearator in the combobox for the filters"""
        return model.get_value(iter, 1)

    def setupDbus(self):
        """ this sets up a dbus listener if none is installed alread """
        # check if there is another g-a-i already and if not setup one
        # listening on dbus
        try:
            bus = dbus.SessionBus()
        except:
            print "warning: could not initiate dbus"
            return
        proxy_obj = bus.get_object('org.freedesktop.AppInstall', '/org/freedesktop/AppInstallObject')
        iface = dbus.Interface(proxy_obj, 'org.freedesktop.AppInstallIFace')
        try:
            iface.bringToFront()
            #print "send bringToFront"
            sys.exit(0)
        except dbus.DBusException, e:
            print "no listening object (%s) "% e
            bus_name = dbus.service.BusName('org.freedesktop.AppInstall',bus)
            self.dbusControler = AppInstallDbusControler(self, bus_name)

    def setBusy(self, flag):
        """ Show a watch cursor if the app is busy for more than 0.3 sec.
            Furthermore provide a loop to handle user interface events """
        if self.window_main.window is None:
            return
        if flag == True:
            self.window_main.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.WATCH))
        else:
            self.window_main.window.set_cursor(None)
        while gtk.events_pending():
            gtk.main_iteration()


    def on_combobox_filter_changed(self, combobox):
        """The filter for the application list was changed"""
        self.setBusy(True)
        active = combobox.get_active()
        model = combobox.get_model()
        iter = model.get_iter(active)
        filter = model.get_value(iter, 3)
        if filter in range(7):
            self.config.set_int("/apps/gnome-app-install/filter_applications", filter)
            self.menu.filter = filter
            self.menu._refilter()
        if len(self.menu.treeview_packages.get_model()) == 0:
             self.show_no_results_msg()
        else:
             self.menu.treeview_packages.set_cursor(0)
        tooltip = model.get_value(iter, 2)
        self.tooltips.set_tip(self.eventbox_filter, tooltip)
        self.setBusy(False)

    def on_window_main_key_press_event(self, widget, event):
        #print "on_window_main_key_press_event()"
        # from /usr/include/gtk-2.0/gdk/gdkkeysyms.h
        GDK_q = 0x071
        if (event.state & gtk.gdk.CONTROL_MASK) and event.keyval == GDK_q:
            self.on_window_main_delete_event(self.window_main, None)

    def error_not_available(self, item):
         """Show an error message that the application cannot be installed"""
         header = _("%s cannot be installed on your "
                    "computer type (%s)") % (item.name,
                                             self.cache.getArch())
         msg = _("Either the application requires special hardware features "
                 "or the vendor decided to not support your computer type.")
         d = gtk.MessageDialog(parent=self.window_main,
                               flags=gtk.DIALOG_MODAL,
                               type=gtk.MESSAGE_ERROR,
                               buttons=gtk.BUTTONS_CLOSE)
         d.set_title("")
         d.set_markup("<big><b>%s</b></big>\n\n%s" % (header, msg))
         d.realize()
         d.window.set_functions(gtk.gdk.FUNC_MOVE)
         d.run()
         d.destroy()

    def tooltip_on_filter(self, cell_view, cell_renderer, model, iter):
        """
        Show a disclaimer in the tooltips of the filters
        """
        id = model.get_path(iter)[0]
        item_text = model.get_value(iter, 0)
        item_disclaimer = model.get_value(iter, 2)
        cell_renderer.set_property('text', item_text)
        cell_parent = cell_view.get_parent()

        if isinstance(cell_parent, gtk.MenuItem) \
           and (cell_parent not in self.tipmap \
                or self.tipmap[cell_parent] != item_disclaimer):
            self.tipmap[cell_parent] = item_disclaimer
            self.tooltips.set_tip(cell_parent, item_disclaimer)

    # install toggle on the treeview
    def on_install_toggle(self, renderer, path):
        #print "on_install_toggle: %s %s" % (renderer, path)
        model = self.treeview_packages.get_model()
        (name, item, popcon) = model[path]
        # first check if the operation is save
        pkg = item.pkgname
        if not (self.cache.has_key(pkg) and \
           self.cache[pkg].candidateDownloadable):
            for it in self.cache._cache.FileList:
                if it.Component != "" and it.Component == item.component:
                    # FIXME: use aptsources to check if no internet sources
                    #        and only cdrom sources are used. if yes continue
                    #        and if not report an error and provide the 
                    #        possibility to apt-get update
                    self.error_not_available(item)
                    return False
            self.saveState()
            if self.addChannel(item):
                self.last_toggle = name
                self.restoreState()
            return
        if self.cache[pkg].isInstalled:
            # check if it can be removed savly
            # FIXME: Provide a list of the corresponding packages or
            #        apps
            self.cache[pkg].markDelete(autoFix=False)
            if self.cache._depcache.BrokenCount > 0:
                d = gtk.MessageDialog(parent=self.window_main,
                                      flags=gtk.DIALOG_MODAL,
                                      type=gtk.MESSAGE_ERROR,
                                      buttons=gtk.BUTTONS_CLOSE)
                d.set_title("")
                d.set_markup("<big><b>%s</b></big>\n\n%s" % \
                             ((_("Cannot remove '%s'") % pkg),
                              (_("One or more applications depend on %s. "
                                "To remove %s and the dependent applications, "
                                "use the Synaptic package manager.") % (pkg, 
                                                                        pkg))))
                d.realize()
                d.window.set_functions(gtk.gdk.FUNC_MOVE)
                d.run()
                d.destroy()
                self.cache.clean()
                return
            self.cache[pkg].markKeep()
            # FIXME: those assert may be a bit too strong,
            # we may just rebuild the cache if something is
            # wrong
            assert self.cache._depcache.BrokenCount == 0
            assert self.cache._depcache.DelCount == 0
        else:
            # check if it can be installed savely
            apt_error = False
            try:
                self.cache[pkg].markInstall(autoFix=True)
            except SystemError:
                apt_error = True
            if self.cache._depcache.BrokenCount > 0 or \
               self.cache._depcache.DelCount > 0 or apt_error:
                # FIXME: Resolve conflicts
                d = gtk.MessageDialog(parent=self.window_main,
                                      flags=gtk.DIALOG_MODAL,
                                      type=gtk.MESSAGE_ERROR,
                                      buttons=gtk.BUTTONS_CLOSE)
                d.set_title("")
                d.set_markup("<big><b>%s</b></big>\n\n%s" % (
                             (_("Cannot install '%s'") % pkg),
                             (_("This application conflicts with other "
                                "installed software. To install '%s' "
                                "the conflicting software "
                                "must be removed before.\n\n"
                                "Switch to the advanced mode to resolve this "
                                "conflict.") % pkg)))
                d.realize()
                d.window.set_functions(gtk.gdk.FUNC_MOVE)
                d.run()
                d.destroy()
                # reset the cache
                # FIXME: a "pkgSimulateInstall,remove"  thing would
                # be nice
                self.cache.clean()
                # FIXME: those assert may be a bit too strong,
                # we may just rebuild the cache if something is
                # wrong
                assert self.cache._depcache.BrokenCount == 0
                assert self.cache._depcache.DelCount == 0
                return
        # the status of the selected package
        status = item.toInstall
        # check if the package provides multiple desktop applications
        if len(self.menu.pkg_to_app[item.pkgname]) > 1:
            apps = self.menu.pkg_to_app[item.pkgname]
            # hack: redraw the treeview (to update the toggle icons after the
            #       tree-model was changed)
            self.treeview_packages.queue_draw()
            # show something to the user (if he hasn't already seen it)
            if not item.pkgname in self.multiple_pkgs_seen:
                dia = DialogMultipleApps(self.datadir, self.window_main,
                                         apps, item.name,
                                         self.cache[pkg].isInstalled)
                rt = dia.run()
                dia.hide()
                self.multiple_pkgs_seen.add(item.pkgname)
                if rt != gtk.RESPONSE_OK:
                    return
            for app in apps:
                 app.toInstall = not status
        else:
            # invert the current selection
            item.toInstall = not status

        self.button_apply.set_sensitive(self.menu.isChanged())

    def addChannel(self, item):
        """Ask for confirmation to add the missing channel or
           component of the current selected application"""
        if item.thirdparty and item.channel:
            dia = DialogProprietary(self.datadir, self.window_main, item)
        else:
            dia = DialogUnavailable(self.datadir, self.window_main, item)
        res = dia.run()
        dia.hide()
        # the user canceld
        if res != gtk.RESPONSE_OK:
            return False
        # let go
        if item.component:
            self.enableComponent(item.component)
            # FIXME: make sure to fix this after release in a more elegant
            #        and generic way
            # we check if it is multiverse and if it is and we don't
            # have universe already, we add universe too (because
            # multiverse depends on universe)
            if item.component == "multiverse":
                for it in self.cache._cache.FileList:
                    if it.Component != "" and it.Component == "universe":
                        break
                else:
                    self.enableComponent("universe")
        elif item.channel:
            self.enableChannel(item.channel)
        else:
            # should never happen
            print "ERROR: addChannel() called without channel or component"
            return False
        # now do the reload
        self.reloadSources()
        return True


    def setupTreeview(self):
        def popcon_view_func(cell_layout, renderer, model, iter, self):
            """
            Create a pixmap showing a row of stars representing the popularity
            of the corresponding application
            """
            (name, item, popcon) = model[iter]
            rank = int((5 * item.popcon / self.menu.popcon_max))
            pix_rating = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True,
                                        8, 96, 16) # depth, width, height
            pix_rating.fill(0x0)
            for i in range(5):
                if not i > rank:
                    self.pixbuf_star.copy_area(0,0,        # from
                                               16,16,      # size
                                               pix_rating, # to-pixbuf
                                               20 * i, 0)  # dest
                else:
                    break
            renderer.set_property("pixbuf", pix_rating)

        def package_view_func(cell_layout, renderer, model, iter):
            app = model.get_value(iter, COL_ITEM)
            name = app.name
            desc = app.description
            current = app.isInstalled
            future = app.toInstall
            available = app.available
            if current != future:
                markup = "<b>%s</b>\n<small><b>%s</b></small>" % (name, desc)
            else:
                markup = "%s\n<small>%s</small>" % (name, desc)
            renderer.set_property("markup", markup)

        def toggle_cell_func(column, cell, model, iter):
            menuitem = model.get_value(iter, COL_ITEM)
            cell.set_property("active", menuitem.toInstall)
            if menuitem.architectures and \
               self.cache.getArch() not in menuitem.architectures:
                cell.set_property("activatable", False)
            else:
                cell.set_property("activatable", True)

        def icon_cell_func(column, cell, model, iter):
            menuitem = model.get_value(iter, COL_ITEM)
            if menuitem == None or menuitem.iconname == None:
                cell.set_property("pixbuf", None)
                cell.set_property("visible", False)
                return
            icon = menuitem.icontheme._getIcon(menuitem.iconname, 24)
            cell.set_property("pixbuf", icon)
            cell.set_property("visible", True)

        # popcon renderer
        renderer_popcon = gtk.CellRendererPixbuf()
        renderer_popcon.set_property("xpad", 4)
        column_app_popcon = gtk.TreeViewColumn(_("Popularity"), 
                                               renderer_popcon)
        #column_app_popcon.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
        column_app_popcon.set_sort_column_id(COL_POPCON)
        column_app_popcon.set_cell_data_func(renderer_popcon, 
                                             popcon_view_func, self)
        self.pixbuf_star = self.icons.load_icon("gnome-app-install-star", 16, 0)

        # check boxes
        renderer_status = gtk.CellRendererToggle()
        renderer_status.connect('toggled', self.on_install_toggle)
        renderer_status.set_property("xalign", 0.3)
        column_app_status = gtk.TreeViewColumn("")
        #column_app_status.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
        column_app_status.pack_start(renderer_status, False)
        column_app_status.set_cell_data_func (renderer_status, 
                                              toggle_cell_func)

        # Application column (icon, name, description)
        column_app = gtk.TreeViewColumn(_("Application"))
        column_app.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
        column_app.set_expand(True)
        column_app.set_sort_column_id(COL_NAME)
        # The icon
        renderer_app_icon = gtk.CellRendererPixbuf()
        column_app.pack_start(renderer_app_icon, False)
        column_app.set_cell_data_func(renderer_app_icon, icon_cell_func)
        # app name and description
        renderer_app = gtk.CellRendererText()
        renderer_app.set_property("ellipsize", pango.ELLIPSIZE_END)
        column_app.pack_start(renderer_app, True)
        column_app.add_attribute(renderer_app, "markup", COL_NAME)
        column_app.set_cell_data_func (renderer_app, package_view_func)

        self.treeview_packages.append_column(column_app_status)
        self.treeview_packages.append_column(column_app)
        self.treeview_packages.append_column(column_app_popcon)
        #FIXME: all columns have to be of fixed nature, but
        #       this doesn't work for column_app_popcon and column_app_status
        #self.treeview_packages.set_fixed_height_mode(True)

        # categories
        column_cat = gtk.TreeViewColumn("")
        # icons
        renderer_cat_icon = gtk.CellRendererPixbuf()
        column_cat.pack_start(renderer_cat_icon, False)
        column_cat.set_cell_data_func(renderer_cat_icon, icon_cell_func)
        # categoriy name
        renderer_cat_name = gtk.CellRendererText()
        renderer_cat_name.set_property("scale", 1.0)
        column_cat.pack_start(renderer_cat_name, True)
        column_cat.add_attribute(renderer_cat_name, "markup", COL_CAT_NAME)
        self.treeview_categories.append_column(column_cat)
        self.treeview_categories.set_search_column(COL_CAT_NAME)

    def saveState(self):
        """ save the current state of the app """
        # store the pkgs that are marked for removal or installation
        (self.to_add, self.to_rm) = self.menu.getChanges()
        (self.cursor_categories_path,x) = self.treeview_categories.get_cursor()
        model = self.treeview_packages.get_model()
        (packages_path, x) = self.treeview_packages.get_cursor()
        if packages_path:
            it = model.get_iter(packages_path)
            self.cursor_pkgname = model.get_value(it, COL_NAME)
        else:
            self.cursor_pkgname = None

    def restoreState(self):
        """ restore the current state of the app """
        # set category
        self.treeview_categories.set_cursor(self.cursor_categories_path)
        model = self.treeview_packages.get_model()
        # reapply search
        query = self.search_entry.get_text()
        if query:
            self.on_search_timeout()
        # remark all packages that were marked for installation
        for item in self.to_add:
            if self.cache.has_key(item.pkgname):
                try:
                    self.cache[item.pkgname].markInstall(autoFix=True)
                except SystemError:
                    continue
                # set the state of the corresponing apps
                apps = self.menu.pkg_to_app[item.pkgname]
                for app in apps:
                    app.toInstall = item.toInstall
        # remark all packages that were marked for removal
        for item in self.to_rm:
            if self.cache.has_key(item.pkgname):
                try:
                    self.cache[item.pkgname].markDelete(autoFix=True)
                except SystemError:
                    continue
                # set the state of the corresponing apps
                apps = self.menu.pkg_to_app[item.pkgname]
                for app in apps:
                    app.toInstall = item.toInstall
        # redraw the treeview so that all check buttons are updated
        self.treeview_packages.queue_draw()

        # find package
        for it in iterate_list_store(model,model.get_iter_first()):
            name = model.get_value(it, COL_NAME)
            # if the app corresponds to the one toggled before toggle it again
            if name == self.last_toggle:
                self.last_toggle = None
                path = model.get_path(it)
                self.treeview_packages.set_cursor(path)
                self.on_install_toggle(None, path)
                break
            # if the app correpsonds to the one selected before select it again
            if name == self.cursor_pkgname and self.last_toggle == None:
                path = model.get_path(it)
                self.treeview_packages.set_cursor(path)
                break

    def updateCache(self, filter=SHOW_ONLY_MAIN):
        self.window_main.set_sensitive(False)
        self.setBusy(True)
            
        progress = GtkOpProgressWindow(self.glade,self.window_main)
        try:
            self.cache = MyCache(progress)
        except Exception, e:
            # show an error dialog if something went wrong with the cache
            header = _("Failed to check for installed and available applications")
            msg = _("This is a major failure of your software " \
                    "management system. Check the file permissions and "\
                    "correctness of the file '/etc/apt/sources.list' and "\
                    "reload the software information: 'sudo apt-get update'.")
            print e
            d = gtk.MessageDialog(parent=self.window_main,
                                  flags=gtk.DIALOG_MODAL,
                                  type=gtk.MESSAGE_ERROR,
                                  buttons=gtk.BUTTONS_CLOSE)
            d.set_title("")
            d.set_markup("<big><b>%s</b></big>\n\n%s" % (header, msg))
            d.realize()
            d.window.set_functions(gtk.gdk.FUNC_MOVE)
            d.run()
            d.destroy()
            sys.exit(1)

        self.menu = ApplicationMenu(self.desktopdir,
                                    self.cachedir,
                                    self.cache,
                                    self.treeview_categories,
                                    self.treeview_packages,
                                    progress, filter)

        # move to "All" category per default
        self.treeview_categories.set_cursor((0,))

        adj = self.scrolled_window.get_vadjustment()
        adj.set_value(0)

        self.setBusy(False)
        self.window_main.set_sensitive(True)
    
    def ignoreChanges(self):
        """
        If any changes have been made, ask the user to apply them and return
        a value based on the status.
        Returns True if the changes should be thrown away and False otherwise
        """
        if not self.menu.isChanged():
            return True
        (to_add, to_rm) = self.menu.getChanges()
        # FIXME: move this set_markup into the dialog itself
        dia = DialogPendingChanges(self.datadir, self.window_main,
                                   to_add, to_rm)
        header =_("Apply changes to installed applications before closing?")
        msg = _("If you do not apply your changes they will be lost "\
                "permanently.")
        dia.label_pending.set_markup("<big><b>%s</b></big>\n\n%s" % \
                                     (header, msg))
        dia.button_ignore_changes.set_label(_("_Close Without Applying"))
        dia.button_ignore_changes.show()
        dia.dialog_pending_changes.realize()
        dia.dialog_pending_changes.window.set_functions(gtk.gdk.FUNC_MOVE)
        res = dia.run()
        dia.hide()
        return res

    
    # ----------------------------
    # Main window button callbacks
    # ----------------------------
    
    def on_button_help_clicked(self, widget):
        subprocess.Popen(["/usr/bin/yelp", "ghelp:gnome-app-install"])

    def applyChanges(self, final=False):
        #print "do_apply()"
        (to_add, to_rm) = self.menu.getChanges()
        # Set a busy cursor
        self.setBusy(True)
        # Get the selections delta for the changes and apply them
        ret = self.packageWorker.perform_action(self.window_main, to_add, to_rm)

        # gcoronel: Commented for guadalinex-app-install integration.
        # error from gksu
        #if ret != 0:
        #    self.setBusy(False)
        #    return False
        
        # Reload the APT cache and treeview
        if final != True:
            self.updateCache(filter=self.menu.filter)

        self.button_apply.set_sensitive(self.menu.isChanged())
        
        # Show window with newly installed programs
        #self.checkNewStore() # only show things that successfully installed
        if len(to_add) > 0:
            dia = DialogNewlyInstalled(self.datadir, self.window_main,
                                       to_add, self.cache)
            dia.run()
            dia.hide()
        
        # And reset the cursor
        self.setBusy(False)
        return True

    def on_button_ok_clicked(self, button):
        # nothing changed, exit
        if not self.menu.isChanged():
            self.quit()
        # something changed, only exit if the changes have
        # succesfully been applied (otherwise cancel)
        if self.confirmChanges():
            if self.applyChanges(final=True):
	        if self.menu.mimeSearch: self.menu.mimeSearch.retry_open()
                self.quit()

    def confirmChanges(self):
        (to_add, to_rm) = self.menu.getChanges()
        dia = DialogPendingChanges(self.datadir, self.window_main,
                                   to_add, to_rm)
        # FIXME: move this inside the dialog class, we show a different
        # text for a quit dialog and a approve dialog
        header = _("Apply the following changes?")
        msg = _("Please take a final look through the list of "\
                "applications that will be installed or removed.")
        dia.label_pending.set_markup("<big><b>%s</b></big>\n\n%s" % \
                                     (header, msg))
        res = dia.run()
        dia.hide()
        if res != gtk.RESPONSE_APPLY:
            # anything but ok makes us leave here
            return False
        else:
            return True

    def on_button_apply_clicked(self, button):
        ret = self.confirmChanges()
        if ret == True :
            self.applyChanges()

    def on_search_timeout(self):
        """
        Filter and sort by rank being based on the entered terms. Otherwise 
        sort by name
        """
        self.setBusy(True)
        query = self.search_entry.get_text()
        model = self.treeview_packages.get_model()
        model.set_default_sort_func(None)
        if query.lstrip() != "":
            self.menu.searchTerms = query.lower().split(" ")
            self.sort_by_ranking = True
            model.set_default_sort_func(self.menu._ranking_sort_func)
            model.set_sort_column_id(-1, gtk.SORT_ASCENDING)
        else:
            self.menu.searchTerms = []
            self.sort_by_ranking = False
            model.set_sort_column_id(COL_NAME, gtk.SORT_ASCENDING)
        self.menu._refilter()
        # FIXME: should be different for "show all applications"
        if len(model) == 0:
            self.show_no_results_msg()
        else:
            self.menu.treeview_packages.set_cursor(0)
        self.setBusy(False)

    def on_search_entry_changed(self, widget):
        """
        Call the actual search method after a small timeout to allow the user
        to enter a longer word
        """
        if self.search_timeout_id > 0:
            gobject.source_remove(self.search_timeout_id)
        self.search_timeout_id = gobject.timeout_add(500,self.on_search_timeout)

    def on_button_clear_clicked(self, button):
        self.search_entry.set_text("")
        # reset the search
        self.menu.search(None)
        # Point the treeview back to the original store
        #self.treeview_packages.set_model(self.menu.store)
        #self.treeview_packages.set_rules_hint(False)
        self.button_clear.set_sensitive(False)

    def on_item_about_activate(self, button):
        from Version import VERSION
        self.dialog_about.set_version(VERSION)
        self.dialog_about.run()
        self.dialog_about.hide()

    def on_reload_activate(self, item):
        self.reloadSources()
        
    def on_button_cancel_clicked(self, item):
        self.quit()

                
    def reloadSources(self):
        self.window_main.set_sensitive(False)
        ret = self.packageWorker.perform_action(self.window_main,
                                                action=PackageWorker.UPDATE)
        self.updateCache(filter=self.menu.filter)
        self.window_main.set_sensitive(True)
        return ret

    def enableChannel(self, channel):
        """ enables a channel with 3rd party software """
        # enabling a channel right now is very easy, just copy it in place
        channelpath = "%s/%s.list" % (self.channelsdir,channel)
        channelkey = "%s/%s.key" % (self.channelsdir,channel)
        if not os.path.exists(channelpath):
            print "WARNING: channel '%s' not found" % channelpath
            return
        #shutil.copy(channelpath,
        #            apt_pkg.Config.FindDir("Dir::Etc::sourceparts"))
        cmd = ["gksu",
               "--desktop", "/usr/share/applications/gnome-app-install.desktop",
               "--",
               "cp", channelpath,
               apt_pkg.Config.FindDir("Dir::Etc::sourceparts")]
        subprocess.call(cmd)
        # install the key as well
        if os.path.exists(channelkey):
            cmd = ["gksu",
                   "--desktop",
                   "/usr/share/applications/gnome-app-install.desktop",
                   "--",
                   "apt-key", "add",channelkey]
            subprocess.call(cmd)
                
    def enableComponent(self, component):
        """ Enables a component of the current distribution
            (in a seperate file in /etc/apt/sources.list.d/$dist-$comp)
        """
        # get the codename of the currently used distro
        pipe = os.popen("lsb_release -c -s")
        distro = pipe.read().strip()
        del pipe
        # sanity check
        if component == "":
            print "no repo found in enableRepository"
            return

        # first find the master mirror, FIXME: something we should
        # shove into aptsources.py?
        mirror = "http://archive.ubuntu.com/ubuntu"
        sources = SourcesList()
        newentry_sec = ""
        newentry_updates = ""
        for source in sources:
            if source.invalid or source.disabled:
                continue
            #print "checking: %s" % source.dist
            # check if the security updates are enabled
            # if yes add the components to the security updates
            if source.dist == ("%s-updates" % distro):
                if component in source.comps:
                    newentry_updates = ""
                else:
                    newentry_updates = "deb %s %s-updates %s\n" % (source.uri,
                                                                   distro,
                                                                   component)
            if source.dist == ("%s-security" % distro):
                #print "need to enable security as well"
                if component in source.comps:
                    newentry_sec = ""
                else:
                    newentry_sec = "deb http://security.ubuntu.com/ubuntu "\
                                   "%s-security %s\n" % (distro, 
                                                         component)
            if source.uri != mirror and is_mirror(mirror,source.uri):
                mirror = source.uri

        newentry = "# automatically added by gnome-app-install on %s\n" % \
                   datetime.today()
        newentry += "deb %s %s %s\n" % (mirror, distro, component)
        if newentry_sec != "":
            newentry += newentry_sec
        if newentry_updates != "":
            newentry += newentry_updates
        channel_dir = apt_pkg.Config.FindDir("Dir::Etc::sourceparts")
        channel_file = "%s-%s.list" % (distro, component)

        channel = tempfile.NamedTemporaryFile()
        channel.write(newentry)
        channel.flush()
        #print "copy: %s %s" % (channel.name, channel_dir+channel_file)
        cmd = ["gksu", "--desktop",
               "/usr/share/applications/gnome-app-install.desktop",
               "--",
               "install","-m","644","-o","0",
               channel.name, channel_dir+channel_file]
        #print cmd
        subprocess.call(cmd)
            
        
    
    # ---------------------------
    # Window management functions
    # ---------------------------
    

    def on_window_main_delete_event(self, window, event):
        if window.get_property("sensitive") == False:
            return True
        if self.menu.isChanged():
            ret = self.ignoreChanges()
            if ret == gtk.RESPONSE_APPLY:
                if not self.applyChanges(final=True):
                    return True
            elif ret == gtk.RESPONSE_CANCEL:
                return True
            elif ret == gtk.RESPONSE_CLOSE:
                self.quit()
        self.quit()

    def on_window_main_destroy_event(self, data=None):
        #if self.window_installed.get_property("visible") == False:
        #    self.quit()
        self.quit()
            
    def quit(self):
        gtk.main_quit()
        sys.exit(0)
    
    def show_description(self, item):
        """Collect and show some information about the package that 
           contains the selected application"""
        details = []
        clean_desc = ""
        short_desc = ""
        version = ""
        desktop_environment = ""
        icons = []

        if self.cache.has_key(item.pkgname):
            version = self.cache[item.pkgname].candidateVersion
            # try to guess the used desktop environment
            for dependencies in desktop_environment_mapping:
                for dep in dependencies:
                    if self.cache.pkgDependsOn(item.pkgname, dep):
                        details.append(desktop_environment_mapping[dependencies][0] % item.name)
                        if desktop_environment_mapping[dependencies][1] != None:
                            icons.append([desktop_environment_mapping[dependencies][1], desktop_environment_mapping[dependencies][0] % item.name])
                        break

        if item.available:
            pkg = self.cache[item.pkgname]
            rough_desc = pkg.description.rstrip(" \n\t")
            # the first line is the short description
            first_break = rough_desc.find("\n")
            short_desc = rough_desc[:first_break].rstrip("\n\t ")
            rough_desc = rough_desc[first_break + 1:].lstrip("\n\t ")
                    
            # so some regular expression magic on the description
            #print "\n\nAdd a newline before each bullet:\n"
            p = re.compile(r'^(\s|\t)*(\*|0|-)',re.MULTILINE)
            rough_desc = p.sub('\n*', rough_desc)
            #print rough_desc

            #print "\n\nreplace all newlines by spaces\n"
            p = re.compile(r'\n', re.MULTILINE)
            rough_desc = p.sub(" ", rough_desc)
            #print rough_desc

            #print "\n\nreplace all multiple spaces by newlines:\n"
            p = re.compile(r'\s\s+', re.MULTILINE)
            rough_desc = p.sub("\n", rough_desc)

            lines = rough_desc.split('\n')
            #print "\n\nrough: \n"
            #print rough_desc

            for i in range(len(lines)):
                if lines[i].split() == []:
                    continue
                first_chunk = lines[i].split()[0]
                if first_chunk == "*":
                    p = re.compile(r'\*\s*', re.MULTILINE)
                    lines[i] = p.sub("", lines[i])
                    clean_desc += "• %s\n" % lines[i]
                else:
                    clean_desc += "%s\n" % lines[i]
            #print clean_desc
        else:
            msg = _("%s cannot be installed" % item.name)
            # check if we have seen the component
            for it in self.cache._cache.FileList:
                # FIXME: we need to exclude cdroms here. the problem is
                # how to detect if a PkgFileIterator is pointing to a cdrom
                if (it.Component != "" and it.Component == item.component) or\
                   (item.architectures and \
                    not self.cache.getArch() in item.architectures):
                    # warn that this app is not available on this plattform
                    details.append(_("%s cannot be installed on your "
                                     "computer type (%s). Either the "
                                     "application requires special "
                                     "hardware features or the vendor "
                                     "decided to not support your "
                                     "computer type.") % (item.name, 
                                     self.cache.getArch()))
                    break
        # A short statement about the freedom and legal status of the 
        # application
        if item.component == "universe":
            care_about_freedom =_("This application is provided by the "
                                  "Ubuntu community.")
            icons.append(["application-community", care_about_freedom])
        elif item.component == "multiverse" or item.thirdparty:
            care_about_freedom = _("The use, modification and distribution "
                                   "of %s is restricted by copyright or by "
                                   "legal terms in some countries.") \
                                   % item.name
            icons.append(["application-proprietary", care_about_freedom])
        elif item.thirdparty or item.channel:
            care_about_freedom = ("%s is provided by a third party vendor "
                                  "and is therefore not an official part "
                                  "of Ubuntu. The third party vendor is "
                                  "responsible for support and security "
                                  "updates.") % item.name
            icons.append(["application-proprietary", care_about_freedom])
        elif item.component == "main" or item.supported:
            icons.append(["application-supported", 
                          _("Canonical Ltd. provides technical support and "
                            "security updates for %s") % item.name])
            care_about_freedom = ""
        else:
            care_about_freedom = ""

        # mutliple apps per pkg check
        s = ""
        if self.menu.pkg_to_app.has_key(item.name) and \
           len(self.menu.pkg_to_app[item.name]) > 1:
            s = _("This application is bundled with "
                  "the following applications: ")
            apps = self.menu.pkg_to_app[item.name]
            s += ", ".join([pkg.name for pkg in apps])
            details.append(s)

        # init the textview that shows the description of the app
        buffer = self.textview_description.get_buffer()
        buffer.set_text("")
        # remove all old tags
        tag_table = buffer.get_tag_table()
        tag_table.foreach((lambda tag, table: table.remove(tag)), tag_table)
        iter = buffer.get_start_iter()
        # we need the default font size for the vertically justification
        pango_context = self.textview_description.get_pango_context()
        font_desc = pango_context.get_font_description()
        font_size = font_desc.get_size() / pango.SCALE
        if item.iconname != "":
            # justify the icon and the app name
            if font_size * pango.SCALE_LARGE > 32:
                icon_size = 32
                adjust_vertically = 0
            elif font_size * pango.SCALE_LARGE < 10:
                icon_size = 24
                adjust_vertically = (icon_size - font_size * pango.SCALE_LARGE)\
                                    /2
            else:
                icon_size = 32
                adjust_vertically = (icon_size - font_size * pango.SCALE_LARGE)\
                                    / 2
            tag_name = buffer.create_tag("app-icon",
                                         right_margin=6,
                                         pixels_above_lines=6)
            tag_name = buffer.create_tag("app-name",
                                         rise=adjust_vertically * pango.SCALE,
                                         pixels_above_lines=6,
                                         weight=pango.WEIGHT_BOLD,
                                         scale=pango.SCALE_LARGE)
            icon_pixbuf = item.icontheme._getIcon(item.iconname, icon_size)
            buffer.insert_pixbuf(iter, icon_pixbuf)
            (start_iter, end_iter,) = buffer.get_bounds()
            buffer.apply_tag_by_name("app-icon", start_iter, end_iter)
            buffer.insert_with_tags_by_name(iter, " %s" % item.name, "app-name")
        else:
            tag_name = buffer.create_tag("app-name",
                                         weight=pango.WEIGHT_BOLD,
                                         pixels_above_lines=6,
                                         scale=pango.SCALE_LARGE)
            buffer.insert_with_tags_by_name(iter, "%s" % item.name, "app-name")
        if short_desc != "": 
            tag_name = buffer.create_tag("short-desc",
                                         weight=pango.WEIGHT_BOLD)
            buffer.insert_with_tags_by_name(iter,
                                            "\n%s" % short_desc,
                                            "short-desc")
            for emblem in icons:
                image_emblem = gtk.Image()
                image_emblem.set_from_icon_name(emblem[0],
                                                gtk.ICON_SIZE_MENU)
                image_emblem.set_pixel_size(16)
                event = gtk.EventBox()
                # use the base color of the textview for the image
                style = self.textview_description.get_style()
                event.modify_bg(gtk.STATE_NORMAL,
                                style.base[gtk.STATE_NORMAL])
                event.add(image_emblem)
                self.tooltips.set_tip(event, emblem[1])
                buffer.insert(iter, " ")
                anchor = buffer.create_child_anchor(iter)
                self.textview_description.add_child_at_anchor(event,
                                                              anchor)
                event.show()
                image_emblem.show()
        elif care_about_freedom != "": 
            buffer.insert(iter, "\n%s" % care_about_freedom)
        if clean_desc != "": 
            buffer.insert(iter, "\n%s" % clean_desc)
        if version != "":
            buffer.insert(iter, _("Version: %s (%s)") % (version, item.pkgname))
        if len(details) > 0:
            for x in details:
                buffer.insert(iter, "\n%s" % x)

    def clear_description(self):
        buffer = self.textview_description.get_buffer()
        buffer.set_text("")

    def on_treeview_packages_row_activated(self, treeview, path, view_column):
        iter = treeview.get_model().get_iter(path)
        item = treeview.get_model().get_value(iter, COL_ITEM)
        # We have to do this check manually, since we don't know if the
        # corresponding CellRendererToggle is not activatable
        if item.architectures and \
            self.cache.getArch() not in item.architectures:
            return False
        self.on_install_toggle(None, path)

    def on_treeview_categories_cursor_changed(self, treeview):
        """
        Show the applications that belong to the selected category and
        restore the previos sorting
        """
        path = treeview.get_cursor()[0]
        iter = treeview.get_model().get_iter(path)
        (name, item) = treeview.get_model()[iter]
        # show a busy cursor if the "all" category was selected
        if path == (0,):
            self.setBusy(True)
        # get the sorting of the current app store
        old_model = self.treeview_packages.get_model()
        (sort_column, sort_type) = old_model.get_sort_column_id()
        self.treeview_packages.set_model(item.applications)
        # if we are in search mode, set a default sort function and sort by it
        # if the previous store was sorted by it
        if self.sort_by_ranking:
            if sort_column == None:
                sort_column = -1
                sort_type = gtk.SORT_ASCENDING
            item.applications.set_default_sort_func(self.menu._ranking_sort_func)
        # if we are not in search mode, but the store still has got 
        # a search function remove it and sort by name if the it was 
        # sorted by the default function before
        elif item.applications.has_default_sort_func():
            if sort_column == None:
                sort_column = COL_NAME
                sort_type = gtk.SORT_ASCENDING
            item.applications.set_default_sort_func(None)
        item.applications.set_sort_column_id(sort_column,
                                             sort_type)
        # filter the apps
        self.menu._refilter()

        if len(self.menu.treeview_packages.get_model()) == 0:
            self.show_no_results_msg()
        else:
            self.menu.treeview_packages.set_cursor(0)
        self.setBusy(False)

    def on_treeview_packages_cursor_changed(self, treeview):
        path = treeview.get_cursor()[0]
        iter = treeview.get_model().get_iter(path)

        (name, item, popcon) = treeview.get_model()[iter]
        self.show_description(item)

    def show_no_results_msg(self):
        """ Give the user some hints if the search returned 
            no results"""
        buffer = self.textview_description.get_buffer()
        buffer.set_text("")
        # remove all old tags
        tag_table = buffer.get_tag_table()
        tag_table.foreach((lambda tag, table: table.remove(tag)), tag_table)
        # create a tag for the first line
        tag_header = buffer.create_tag("first-line",
                                       weight = pango.WEIGHT_BOLD,
                                       pixels_above_lines=6)
        msg = _("There is no matching application available.")
        iter = buffer.get_start_iter()
        buffer.insert_with_tags_by_name(iter, msg, "first-line")
        if self.menu.filter != SHOW_ONLY_FREE and \
           self.menu.filter != SHOW_ALL:
            msg = "\n%s" % _("To broaden your search, choose "
                             "'Show all Open Source applications' or "
                             "'Show all available' applications.")
            buffer.insert_with_tags(iter, msg)
        if self.treeview_categories.get_cursor()[0] != (0,):
            msg = "\n%s" % _("To broaden your search, choose "
                             "'All' categories.")
            buffer.insert_with_tags(iter, msg)
        
    def show_intro(self):
        """ Show a quick introduction to gnome-app-install 
            in the description view"""
        buffer = self.textview_description.get_buffer()
        buffer.set_text("")
        iter = buffer.get_start_iter()
        tag_header = buffer.create_tag("header",
                                       scale = pango.SCALE_LARGE,
                                       weight = pango.WEIGHT_BOLD,
                                       pixels_above_lines=6)
        msg = _("To install an application check the box next to the "
                "application. Uncheck the box to remove "
                "the application.") + "\n"
        msg += _("To perform advanced tasks use the "
                 "Synaptic package manager.")
        buffer.insert_with_tags(iter, "%s\n" % _("Quick Introduction"), 
                                tag_header)
        buffer.insert(iter, msg)

# Entry point for testing in source tree
if __name__ == '__main__':
    app = AppInstall(os.path.abspath("menu-data"),
                     os.path.abspath("data"),
                     sys.argv)
    gtk.main()
