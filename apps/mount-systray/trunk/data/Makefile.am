uidir = $(datadir)/mount-systray
ui_DATA = \
	preferences.glade

pixmapsdir = $(datadir)/pixmaps/mount-systray
pixmaps_DATA = \
	usb.png \
	floppy.png \
	sicon.png \
	cdrom.png

schemadir = /etc/gconf/schemas
schema_in_files = mount-systray.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

desktopdir = /etc/xdg/autostart
desktop_in_files = mount-systray.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES = \
	preferences.glade.bak \
	preferences.gladep.bak \
	preferences.gladep \
	mount-systray.desktop \
	mount-systray.schemas

EXTRA_DIST = \
	$(ui_DATA) \
	$(pixmaps_DATA) \
	$(schema_in_files) \
	$(desktop_in_files)

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
