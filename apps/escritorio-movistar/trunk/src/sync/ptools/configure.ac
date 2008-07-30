#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([ptools.h])
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
AC_CHECK_HEADER([pthread.h])
AC_CHECK_DECL([_WIN32], 
	[], 
	[AC_CHECK_HEADERS([windows.h winbase.h winuser.h winobj.h], 
					  [AC_DEFINE([_WIN32], [1], 
					  			 [Define to compile for Win32.])])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_HEADER_STDC
AC_CHECK_FUNCS([memset nl_langinfo setlocale iconv_open iconv iconv_close],
	[], [AC_MSG_ERROR(missing required functions)])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
