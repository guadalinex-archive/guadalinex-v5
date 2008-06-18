INCLUDES = 					   \
	$(CAPPLET_CFLAGS) 			   \
	$(GVM_CFLAGS)			   	   \
	$(NOTIFY_CFLAGS)			   \
	-DDBUS_API_SUBJECT_TO_CHANGE		   \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"   \
	-DBINDIR=\"$(bindir)\"			   \
	-DLIBEXECDIR=\"$(libexecdir)\"		   \
	-DGLADEDIR=\"$(pkgdatadir)\"		   \
	-DDATADIR=\"$(datadir)\"

bin_PROGRAMS = gnome-volume-properties
libexec_PROGRAMS = gnome-volume-manager

gnome_volume_manager_SOURCES = manager.c clipboard.c gvm.h
gnome_volume_manager_LDADD = $(GVM_LIBS) $(NOTIFY_LIBS)

gnome_volume_properties_SOURCES = properties.c gvm.h
gnome_volume_properties_LDADD = $(CAPPLET_LIBS)
