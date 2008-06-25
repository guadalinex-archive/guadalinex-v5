# Process this file with autoconf to produce a configure script.

GEDIT_MAJOR=2.16.0
GEDIT_PLUGINS_MAJOR=2.16.0

AC_INIT(gedit-plugin-TLoleo, 2.16.1, http://www.indisys.es/)

AC_CONFIG_SRCDIR([plugins])
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_PROGRAM([gedit])

AC_SUBST(GEDIT_PLUGINS_MAJOR)

AC_PREREQ(2.59)

AM_INIT_AUTOMAKE([1.8 dist-bzip2 no-dist-gzip])

GNOME_DOC_INIT

AM_MAINTAINER_MODE
AC_CONFIG_HEADERS([config.h])
AC_DISABLE_STATIC

AC_PATH_PROG(GCONFTOOL, gconftool-2)
IT_PROG_INTLTOOL([0.35.0])

AC_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I m4")

# ================================================================
# Gettext stuff.
# ================================================================

GETTEXT_PACKAGE=gedit-plugin-TLoleo
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT

# ================================================================
# Start of pkg-config checks
# ================================================================

PKG_CHECK_MODULES(GEDIT, [
	glib-2.0 >= 2.8.0
	gtk+-2.0 >= 2.8.0
	gtksourceview-2.0 >= 2.0.0	
	gedit-2.20 >= 2.20.0
	libglade-2.0 >= 2.4.0
	gnome-vfs-2.0 >= 2.13.4
])
GEDIT_LIBS="${GEDIT_LIBS}"
AC_SUBST(GEDIT_LIBS)
AC_SUBST(GEDIT_CFLAGS)

# ================================================================
# Plugins
# ================================================================
AC_MSG_CHECKING([which plugins to build])

ALL_PLUGINS="TLoleo"
USEFUL_PLUGINS="TLoleo"
DEFAULT_PLUGINS="TLoleo"

DIST_PLUGINS="$ALL_PLUGINS"

AC_ARG_WITH([plugins],
[  --with-plugins=plugin1,plugin2,...
			  build the specified plugins. Available:
			  TLoleo, as well
			  as the aliases default, all, and really-all],
			  [plugins=$with_plugins],
			  [plugins="default"])

if test "x$with_plugins" = xyes
then
	plugins="default"
fi

explicit_plugins=
PLUGINS=
IFS="${IFS=	}"; geditpl_save_ifs="$IFS"; IFS=","
for pl in $plugins
do
	if echo "$ALL_PLUGINS" | egrep "(^| )$pl(\$| )" > /dev/null
	then
		PLUGINS="$PLUGINS $pl"
		explicit_plugins="$explicit_plugins $pl"
	elif test "x$pl" = "xdefault"
	then
		PLUGINS="$PLUGINS $DEFAULT_PLUGINS"
	elif test "x$pl" = "xall"
	then
		PLUGINS="$PLUGINS $USEFUL_PLUGINS"
	elif test "x$pl" = "xreally-all"
	then
		PLUGINS="$PLUGINS $ALL_PLUGINS"
	else
		AC_MSG_ERROR([the specified plugin $pl does not exist])
	fi
done
IFS="$geditpl_save_ifs"

AC_MSG_RESULT([$PLUGINS])

# ================================================================
# GConf related settings
# ================================================================

AM_GCONF_SOURCE_2


disabled_plugins=
# ================================================================
# Plugin specific checks
# ================================================================
check_plugin_defined() {
	if echo "$2" | egrep "\b($1)\b" > /dev/null;
	then
		return 1
	else
		return 0
	fi
}

plugin_defined() {
	check_plugin_defined "$1" "$PLUGINS"
	return $?
}

plugin_defined_explicit() {
	check_plugin_defined "$1" "$explicit_plugins"
	return $?
}

undef_plugin() {
	if test -z "$disabled_plugins"
	then
		disabled_plugins="$1 ($2)"
	else
		disabled_plugins="$disabled_plugins, $1 ($2)"
	fi
	
	PLUGINS=$(echo "$PLUGINS" | sed -e "s/[[^a-zA-Z0-9_]]$1[[ \t\r\n\f]]\{1,\}/ /" -e "s/[[^a-zA-Z0-9_]]$1$//" -e "s/^$1[[ \t\r\n\f]]\{1,\}//")
}

# ================================================================
# 
# ================================================================
# ================================================================
# TLoleo
# ================================================================
plugin_defined TLoleo
if test "$?" = 1
then
	#Use this checking when TLoleo will be modulate.
	#PKG_CHECK_MODULES([TLOLEO], [
	#	TLoleo >= 0.0.0
	#], have_TLoleo=yes, have_TLoleo=no)

	#Use this variable while TLoleo isn't a version package module
	have_TLoleo=yes

	if test "x$have_TLoleo" = "xno"
	then
		plugin_defined_explicit TLoleo
		if test "$?" = 1
		then
			AC_MSG_ERROR([TLoleo could not be found, needed to built TLoleo plugin])
		else
			AC_MSG_WARN([TLoleo could not be found, TLoleo plugin will be disabled])

			undef_plugin TLoleo "TLoleo not found"
		fi
	fi
fi


if test -z "$disabled_plugins"
then
	disabled_plugins="none"
fi

PLUGINS=$(echo $PLUGINS | tr ' ' '\n' | sort | uniq | tr '\n' ' ')
AC_SUBST([DIST_PLUGINS])
AC_SUBST([BUILD_PLUGINS],[$PLUGINS])

# ================================================================
# Misc
# ================================================================
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
			      
#GNOME_COMPILE_WARNINGS(yes)

AC_ARG_ENABLE(deprecations,
              [AC_HELP_STRING([--enable-deprecations],
                              [warn about deprecated usages [default=no]])],,
              [enable_deprecations=no])

if test "x$enable_deprecations" = "xyes"
then
	DISABLE_DEPRECATED_CFLAGS="\
		-DG_DISABLE_DEPRECATED		\
		-DGDK_DISABLE_DEPRECATED	\
		-DGTK_DISABLE_DEPRECATED	\
		-DGDK_PIXBUF_DISABLE_DEPRECATED	\
		-DGNOME_DISABLE_DEPRECATED"
	AC_SUBST(DISABLE_DEPRECATED_CFLAGS)
fi

PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

AC_CONFIG_FILES([
Makefile
plugins/Makefile
plugins/TLoleo/Makefile
po/Makefile.in])

AC_OUTPUT

echo "

Configuration:

	Source code location:   ${srcdir}
	Compiler:               ${CC}
	Prefix:			${prefix}
	Python Plugins Support: never
	Plugins:                $PLUGINS
	
	Disabled plugins:       $disabled_plugins

Note: you have to install these plugins into the same prefix than your gedit
installation (probably /usr if you're using your distro packages, /usr/local
if you have compiled it on your own).

"
