#!/usr/bin/python
# -*- coding: utf-8 -*-
# Funciones asociadas a configuraciones del usuario.
# Permite configurar el fondo de escritorio e instalar nuevas fuentes true-type.

import os, re, registry, shutil, computer
from folder import *

def set_wallpaper(img):
	"""Copia y configura la imagen que se le pasa como fondo de escritorio.
	Válido para GNOME y KDE"""
	dest = os.path.expanduser('~') #for GNOME should be /usr/share/bankgrounds and for KDE /usr/share/themes but it requires toot privileges
	if os.path.exists(img):
		(folder, filename) = os.path.split(img)
		wallpaper = os.path.join(dest, filename)
		if not os.path.exists(wallpaper):
			try:
				shutil.copy2(img, dest)
			except:
				error("Wallpaper not installed")
		if os.path.exists('/usr/bin/gconftool'): # for GNOME
			os.system("gconftool --type string --set /desktop/gnome/background/picture_filename %s" % wallpaper.replace(' ','\ '))
		if os.path.exists('/usr/bin/dcop'): # for KDE
			os.system('dcop kdesktop KBackgroundIface setWallpaper  %s 4' % wallpaper.replace(' ','\ '))
		print "Wallpaper installed succesfully"
		
			
def import_fonts(fontsdir):
	"""Copia las fuentes ttf del directorio que se le pasa"""
	dest = folder(os.path.join(os.path.expanduser('~'),'.fonts','ttf-windows'))
	fontsdir.copy(dest.path, '.ttf')


if __name__ == "__main__":
	set_wallpaper("/usr/share/backgrounds/ubuntu-smooth-chocolate.png")
	import_fonts(folder("/usr/share/fonts/truetype"))
	
