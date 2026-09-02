AC_INIT([cmom], [1.0], [victor.perticarrari@gmail.com])
AC_CONFIG_SRCDIR([src/main/broker.c])
AC_CONFIG_HEADERS([includes/config.h])
AC_CONFIG_AUX_DIR([build-aux])

# Enable subdir-objects in automake
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])

# Checks for programs
AC_PROG_CC
AM_PROG_AR           # needed for static library

# Checks for headers
AC_CHECK_HEADERS([stdlib.h string.h ucontext.h])

# Checks for library functions
AC_PROG_RANLIB
AC_CHECK_FUNCS([strdup])

# Check for CUnit (testing framework)
PKG_CHECK_MODULES([CUNIT], [cunit],
                  [have_cunit=yes],
                  [have_cunit=no])
AM_CONDITIONAL([HAVE_CUNIT], [test "x$have_cunit" = "xyes"])
if test "x$have_cunit" = "xyes"; then
    AC_DEFINE([HAVE_CUNIT], [1], [Define if CUnit is available])
fi

# Check for log4c (logging library)
PKG_CHECK_MODULES([LOG4C], [log4c], [have_log4c=yes], [have_log4c=no])
AM_CONDITIONAL([HAVE_LOG4C], [test "x$have_log4c" = "xyes"])
if test "x$have_log4c" = "xyes"; then
    AC_DEFINE([HAVE_LOG4C], [1], [Define if log4c is available])
else
    AC_MSG_WARN([log4c not found – logging will fall back to stderr])
fi

# Enable logging by default, but allow --disable-logging
AC_ARG_ENABLE([logging],
    [AS_HELP_STRING([--disable-logging], [disable logging (default: enabled)])],
    [enable_logging=$enableval],
    [enable_logging=yes])
if test "x$enable_logging" = "xyes"; then
    AC_DEFINE([ENABLE_LOGGING], [1], [Define to 1 to enable logging macros])
fi

# Output files
AC_CONFIG_FILES([Makefile
                 src/main/Makefile
                 src/test/Makefile])
AC_OUTPUT