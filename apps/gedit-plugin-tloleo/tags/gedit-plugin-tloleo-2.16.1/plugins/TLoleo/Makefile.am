# TLoleo plugin
plugindir = $(libdir)/gedit-2/plugins

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"

plugin_LTLIBRARIES = libTLoleo.la

libTLoleo_la_SOURCES = \
	gedit-TLoleo-plugin-defs.h	\
	gedit-TLoleo-plugin.h	\
	gedit-TLoleo-plugin.c
	
libTLoleo_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libTLoleo_la_LIBADD  = $(GEDIT_LIBS)

gladedir = $(datadir)/gedit-2/glade
glade_DATA =

plugin_in_files = TLoleo.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

EXTRA_DIST = $(glade_DATA) $(plugin_in_files)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

