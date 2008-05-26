AC_INIT(src/mount-systray.py)
AM_INIT_AUTOMAKE(mount-systray, 0.2)

dnl check for python
dnl AM_PATH_PYTHON
AM_PATH_PYTHON

PKG_CHECK_MODULES(PYGTK, pygtk-2.0 >= 2.6)
AC_SUBST(PYGTK_CFLAGS)
AC_SUBST(PYGTK_LIBS)


AC_PATH_PROG( PKGCONFIG, pkg-config, no, $PATH:/usr/bin:/usr/local/bin)
  if test "$PKGCONFIG" != "no" ; then
    AC_MSG_CHECKING([PYTHONPATH env variable for PyGTK])
    PYTHONPATH=$($PKGCONFIG --variable=pyexecdir pygtk-2.0)
    AC_SUBST([PYTHONPATH])
    AC_MSG_RESULT($PYTHONPATH)
  fi

export PYTHONPATH=$PYGTK_DIR:$PYTHONPATH
dnl check for libglade
AC_MSG_CHECKING(for gtk.glade)
prog="
import sys
try:
  import gtk.glade
except RuntimeError:
  sys.exit(0)
except ImportError:
  sys.exit(1)
sys.exit(0)
"

if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
then
  AC_MSG_RESULT(found)
else
  AC_MSG_RESULT(not found)
  AC_MSG_ERROR([You need to have python libglade bindings installed])
fi

PKG_CHECK_MODULES(GNOMEPYTHON, gnome-python-2.0 >= 2.12.0)

export PYTHONPATH=$PYGTK_DIR:$PYTHONPATH
dnl check for python-gconf
AC_MSG_CHECKING(for python gconf)
prog="
import sys
try:
  import gconf
except RuntimeError:
  sys.exit(0)
except ImportError:
  sys.exit(1)
sys.exit(0)
"

if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
then
  AC_MSG_RESULT(found)
else
  AC_MSG_RESULT(not found)
  AC_MSG_ERROR([You need to have python gconf bindings installed])
fi

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

dnl *******************************
dnl Internationalization
dnl ******************************* 
GETTEXT_PACKAGE=mount-systray
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
dnl AC_CONFIG_MACRO_DIR([m4])
GNOME_DOC_INIT

dnl Add the languages which your application supports here.
ALL_LINGUAS="es"

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL

AC_OUTPUT([
Makefile
src/config.py
src/Makefile
src/mount-systray
po/Makefile.in
data/Makefile
data/mount-systray.desktop.in
])
