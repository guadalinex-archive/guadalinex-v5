# gnome-systray.py

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#python modules
import sys

#custom modules
import config
from MSDeviceManager import MSDeviceManager
from MSConfig import MSConfig
from MSSystray import MSSystray

##
## I18N
##
import gettext, locale
gettext.install(config.GETTEXT_PACKAGE(), config.GNOMELOCALEDIR(), unicode=1)

gettext.bindtextdomain('mount-systray', config.GNOMELOCALEDIR())
if hasattr(gettext, 'bind_textdomain_codeset'):
	gettext.bind_textdomain_codeset('mount-systray','UTF-8')
gettext.textdomain('mount-systray')

locale.bindtextdomain('mount-systray', config.GNOMELOCALEDIR())
if hasattr(locale, 'bind_textdomain_codeset'):
	locale.bind_textdomain_codeset('mount-systray','UTF-8')
locale.textdomain('mount-systray')

try:
    import pygtk
    #tell pyGTK, if possible, that we want GTKv2
    pygtk.require("2.0")
  
except:
    #Some distributions come with GTK2, but not pyGTK
    pass

try:
    import gtk
    import gtk.glade
    import gnome
    import pynotify
  
except:
    print _("You need to install pyGTK or GTKv2,\n"
            "or set your PYTHONPATH correctly.\n"
            "try: export PYTHONPATH= ")
    sys.exit(1)

gnome.program_init("mount-systray", "0.1")

device_manager = MSDeviceManager()
conf = MSConfig()
MSSystray(device_manager, conf)

pynotify.init("mount-systray")
gtk.main()
