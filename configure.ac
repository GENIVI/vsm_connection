#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libvsmsocket], [1.0], [https://github.com/GENIVI/vsm_connection])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset select socket fdopen])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
