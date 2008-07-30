applicationsdir = $(datadir)/application-registry
applications_DATA = escritorio-movistar.applications

mimeinfodir = $(datadir)/mime-info
mimeinfo_DATA = escritorio-movistar.keys escritorio-movistar.mime

mimepackagedir = $(datadir)/mime/packages
mimepackage_DATA = escritorio-movistar.xml

desktopdir = $(datadir)/applications/
desktop_in_files = escritorio-movistar.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

hicolornotifydir=

EXTRA_DIST=$(applications_DATA) $(mimeinfo_DATA) $(mimepackage_DATA) $(desktop_DATA)
