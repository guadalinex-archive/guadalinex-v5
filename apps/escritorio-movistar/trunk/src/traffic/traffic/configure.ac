#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([Traffic.cpp])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_LANG(C++)

# Checks for libraries.
AC_CHECK_LIB([sqlite3], [sqlite3_open])
AC_CHECK_LIB([sqlite3], [sqlite3_exec])
AC_CHECK_LIB([sqlite3], [sqlite3_close])
AC_CHECK_LIB([sqlite3], [sqlite3_finalize])
AC_CHECK_LIB([sqlite3], [sqlite3_column_text])
AC_CHECK_LIB([sqlite3], [sqlite3_prepare])
AC_CHECK_LIB([sqlite3], [sqlite3_step])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([sqlite3.h])
AC_CHECK_HEADERS([deque])
AC_CHECK_HEADERS([vector])
AC_CHECK_HEADERS([sstream])
AC_CHECK_DECL([_WIN32], 
	[], 
	[AC_CHECK_HEADERS([windows.h winbase.h winuser.h winobj.h], 
					  [AC_DEFINE([_WIN32], [1], 
					  			 [Define to compile for Win32.])])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

AC_CHECK_TYPES([sqlite3_stmt *], 
	[], 
	[AC_MSG_ERROR([no proper sqlite 3 environment found])],
	[#ifdef HAVE_SQLITE3_H
#include <sqlite3.h>
#endif])

# Checks for library functions.
AC_HEADER_STDC
AC_CHECK_FUNCS([strtoll],
	[], [AC_MSG_ERROR(missing required functions)])
		
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
