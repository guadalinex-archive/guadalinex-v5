#!/usr/bin/python2.5
# -*- coding: UTF-8 -*-

import pygtk
import gtk
import sys
import os

message_text = """

<b>Bienvenido/a a Guadalinex</b>

Guadalinex no es sólo lo que contiene este CD. Todavía hay más.

¿Desea ver cómo puede instalar más aplicaciones?

"""
check_text = "No volver a mostrar este diálogo"
image_src = "/usr/share/first_start/imagen.jpg"

class FirstStart:

    def __init__(self):
        self.dialog = gtk.Dialog ("Primer inicio", None, 0, 
                                  (gtk.STOCK_NO, gtk.RESPONSE_REJECT,
                                  gtk.STOCK_YES, gtk.RESPONSE_ACCEPT))
	self.mainLabel = gtk.Label()
        self.mainLabel.set_markup(message_text)
        self.checkShow = gtk.CheckButton(check_text)
        self.image = gtk.Image()
        self.image.set_from_file(image_src)
        self.align = gtk.Alignment(0.5, 0.5, 0, 0)

        self.hcontainer = gtk.HBox(False, 10)
        self.hcontainer.set_border_width(5)
        self.hcontainer.pack_start(self.align, False, False, 0)
        self.hcontainer.pack_start(self.image, True, True, 0)
        self.hcontainer.pack_start(self.mainLabel, True, True, 0)
        self.dialog.vbox.pack_start(self.hcontainer, True, True, 0)
        self.dialog.vbox.pack_start(self.checkShow, True, True, 0)

        self.dialog.set_resizable(False)
        self.hcontainer.show()
        self.align.show()
        self.image.show()
        self.mainLabel.show()
        self.checkShow.show()

        self.dialog.connect("destroy", self.destroy)
        self.dialog.connect("response", self.response)
        self.dialog.show()

    def response(self, widget, res):
        if res == gtk.RESPONSE_ACCEPT:
            os.system("firefox file:///usr/share/ayuda.html")
        if self.checkShow.get_active() == True:
            f = open(os.environ['HOME'] + "/.gnome2_private/first_start","w")
            f.write("OK\n")
            f.close()
        print 
        gtk.main_quit()

    def destroy(self, widget, data=None):
        gtk.main_quit()

    def main(self):
        gtk.main()

if __name__ == "__main__":
    try:
        os.readlink("/rofs/initrd.img")
    except:
        try:
            f = open(os.environ['HOME'] + "/.gnome2_private/first_start","r")
            f.close()
        except:
            fs = FirstStart()
            fs.main()
