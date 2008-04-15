AC_PREREQ(2.52)
AC_INIT(ubiquity, 1.8.4, https://launchpad.net/ubuntu/+source/ubiquity/+filebug)
AC_CONFIG_SRCDIR(src/cut-and-paste/e-map/e-map.c)
AC_CONFIG_AUX_DIR([build])
AM_INIT_AUTOMAKE([foreign])

AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

AM_PROG_LIBTOOL

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

IT_PROG_INTLTOOL

AC_ISC_POSIX
AC_PROG_CC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

GTK_REQUIRED=2.10.0

AM_PATH_PYTHON([2.4])
UBIQUITY_CHECK_PYTHON_HEADERS(,[AC_MSG_ERROR([could not find Python headers])])

if test "x$UBIQUITY_NO_GTK" = x; then
  PKG_CHECK_MODULES(PYGTK, [gtk+-2.0 >= $GTK_REQUIRED pygtk-2.0])
  AC_SUBST(PYGTK_CFLAGS)
  AC_SUBST(PYGTK_LIBS)
  AC_PATH_PROG(PYGTK_CODEGEN, [pygtk-codegen-2.0], [no])
else
  AC_SUBST(PYGTK_CFLAGS, [])
  AC_SUBST(PYGTK_LIBS, [])
  AC_SUBST(PYGTK_CODEGEN, [no])
fi

AM_CONDITIONAL([PYGTK_CODEGEN], [test "x$PYGTK_CODEGEN" != xno])
if test "x$PYGTK_CODEGEN" != xno; then
  AC_MSG_CHECKING([for pygtk defs])
  PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
  AC_SUBST(PYGTK_DEFSDIR)
  AC_MSG_RESULT($PYGTK_DEFSDIR)

  AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

  AM_PATH_GTK_2_0([$GTK_REQUIRED])

  PKG_CHECK_MODULES(UBIQUITY_DEPENDS,[
	gtk+-2.0 >= $GTK_REQUIRED
	libart-2.0
	])

  AC_CONFIG_FILES(
	src/Makefile
	src/cut-and-paste/Makefile
	src/cut-and-paste/e-map/Makefile
	)
fi

AC_DEFINE_UNQUOTED(PIXMAPS_DIR, "${datadir}/${PACKAGE}/pixmaps", [pixmaps directory])
pixmapsdir="${datadir}/${PACKAGE}/pixmaps"
AC_SUBST(pixmapsdir)

AC_CONFIG_FILES(
	Makefile
	desktop/Makefile
	pixmaps/Makefile
	po/Makefile.in
	)
AC_OUTPUT
