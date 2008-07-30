#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([SyncML.cpp])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_LANG(C++)

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_mutex_init], [], 
	[AC_MSG_ERROR(missing required library function)])
AC_CHECK_LIB([pthread], [pthread_mutex_lock], [], 
	[AC_MSG_ERROR(missing required library function)])
AC_CHECK_LIB([pthread], [pthread_mutex_unlock], [], 
	[AC_MSG_ERROR(missing required library function)])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h])
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([string vector memory])
AC_CHECK_DECL([_WIN32], 
	[], 
	[AC_CHECK_HEADERS([windows.h winbase.h winuser.h winobj.h], 
					  [AC_DEFINE([_WIN32], [1], 
					  			 [Define to compile for Win32.])])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([floor pow setlocale])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
