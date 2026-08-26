AC_INIT([myproject], [1.0], [you@example.com])
AC_CONFIG_SRCDIR([src/cmom.c])
AC_CONFIG_HEADERS([config.h])

# Use auxiliary files in a 'build-aux' directory (optional but tidy)
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Check for a C compiler
AC_PROG_CC

# Checks for header files (standard C headers)
AC_HEADER_STDC

# Checks for library functions (e.g., malloc, strdup)
AC_CHECK_FUNCS([strdup])

# Generate Makefiles
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT