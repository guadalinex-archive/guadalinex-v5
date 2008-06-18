SUBDIRS = src po

pkgdata_DATA = gnome-volume-properties.glade

schemadir = $(sysconfdir)/gconf/schemas
schema_in_files = gnome-volume-manager.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = gnome-volume-properties.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = gnome-volume-manager.desktop.in
autostart_in_in_files = gnome-volume-manager.desktop.in.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

gnome-volume-manager.desktop.in: $(srcdir)/gnome-volume-manager.desktop.in.in
	sed -e 's,[@]libexecdir[@],$(libexecdir),g' <$(srcdir)/gnome-volume-manager.desktop.in.in \
                >gnome-volume-manager.desktop.in

EXTRA_DIST =				\
	HACKING				\
	gnome-volume-manager.spec.in	\
	$(desktop_in_files)		\
	$(autostart_in_in_files)	\
	$(schema_in_files)		\
	$(pkgdata_DATA)			\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	mkinstalldirs

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)

CLEANFILES = $(desktop_DATA) $(autostart_DATA) $(schema_DATA) $(autostart_in_files)
DISTCLEANFILES = intltool-extract intltool-merge intltool-update
