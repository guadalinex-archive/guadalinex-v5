#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([emsyncmodule.cpp])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX

AC_LANG(C++)

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([locale.h python2.5/Python.h])
AC_CHECK_HEADERS(
	[python2.5/structmember.h], [], [],
	[#ifdef HAVE_PYTHON2_5_PYTHON_H
	 #include <python2.5/Python.h>
	 #endif])
AC_CHECK_HEADERS([string vector])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
