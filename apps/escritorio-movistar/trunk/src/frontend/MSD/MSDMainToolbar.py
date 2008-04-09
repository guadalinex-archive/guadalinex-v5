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
import gtk
import gtk.gdk
import gtk.glade
import math
import cairo
import MSD
import os

class MSDMainToolbar(gtk.HBox):
	def __init__(self):
		gtk.HBox.__init__(self, homogeneous=True, spacing=10)

		self.xml = gtk.glade.XML(os.path.join(MSD.glade_files_dir, "maintoolbar.glade"),
					 root="maintoolbar_alignment")

		self.alignment = self.xml.get_widget("maintoolbar_alignment")

		self.addressbook_button = self.xml.get_widget("addressbook_button")
                image = gtk.Image()
                image.set_from_file(MSD.icons_files_dir + "addressbook_48x48.png")
                self.addressbook_button.set_image(image)


                self.bookmarks_button = self.xml.get_widget("bookmarks_button")
                image = gtk.Image()
                image.set_from_file(MSD.icons_files_dir + "accesos_directos_48x48.png")
                self.bookmarks_button.set_image(image)

                self.preferences_button = self.xml.get_widget("preferences_button")
                image = gtk.Image()
                image.set_from_file(MSD.icons_files_dir + "configuracion_48x48.png")
                self.preferences_button.set_image(image)

                self.help_button = self.xml.get_widget("help_button")
                image = gtk.Image()
                image.set_from_file(MSD.icons_files_dir + u"ayuda_48x48.png")
                self.help_button.set_image(image)
		
		self.pack_start(self.alignment)
		self.connect("expose-event", self.expose)
	
	def expose(self, widget, event):
		#Creamos un contexto de dibujo cairo
		self.context = widget.window.cairo_create()
                self.context.rectangle(event.area.x, event.area.y,
                                       event.area.width, event.area.height)
		self.context.clip()
                
                self.draw(self.context)                
		return False
            
        def draw(self, context):
            rect = self.get_allocation()
            surface = cairo.ImageSurface(cairo.FORMAT_ARGB32,
                                         rect.width,
                                         rect.height)
            self.context.scale (rect.width/1.0, rect.height/1.0)
            self.context.set_source_surface(surface)
            pat = cairo.LinearGradient (0.0, 0.0, 0.0, 1.0)
            pat.add_color_stop_rgba (1, 0.55, 0.69, 0.97, 1)
            pat.add_color_stop_rgba (0, 1, 1, 1, 1)
            
            context.rectangle (0,0,1,1)
            context.set_source (pat)
            context.fill ()
            
