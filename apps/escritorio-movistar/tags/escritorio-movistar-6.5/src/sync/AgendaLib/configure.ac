#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([AgendaLib.cpp])
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
AC_CHECK_LIB([sqlite3], [sqlite3_prepare_v2])
AC_CHECK_LIB([sqlite3], [sqlite3_bind_text])
AC_CHECK_LIB([sqlite3], [sqlite3_step])
AC_CHECK_LIB([sqlite3], [sqlite3_last_insert_rowid])
AC_CHECK_LIB([sqlite3], [sqlite3_reset])
AC_CHECK_LIB([sqlite3], [sqlite3_prepare])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h string.h])
AC_CHECK_HEADERS([sqlite3.h])
AC_CHECK_HEADERS([exception])
AC_CHECK_HEADERS([string])
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

AC_CHECK_TYPES([std::exception], 
	[], 
	[AC_MSG_ERROR([No C++ exceptions])],
	[#ifdef HAVE_EXCEPTION
#include <exception>
#endif])

# Checks for library functions.
AC_MSG_CHECKING([for std::exception::exception(const char *)])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[
		 #ifdef HAVE_EXCEPTION
		 #include <exception>
		 #endif
		], 
		[
			std::exception ex("error");
			const char *msg=ex.what();
		])
	],
	[AC_DEFINE(
		[HAVE_CXX_EXCEPTION_CHAR_P_CTOR], 
		[1], 
		[Define if std::exception::exception(const char*) exists])
	 AC_MSG_RESULT([no])],
	[AC_MSG_RESULT([yes])]	 
)
AC_CHECK_FUNCS(
	[snprintf], [], 
	[
		AC_CHECK_FUNCS(
			[_snprintf], [], 
			[	
				AC_CHECK_FUNCS(
					[sprintf], [], 		
					[AC_MSG_ERROR([Cannot find snprintf, _snprintf, or sprintf])]
				)
			]
		)
	]
)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
