# Only build tests if CUnit was found
if HAVE_CUNIT

# Test programs that should be built (but not installed)
check_PROGRAMS = test_suite

# Sources for the test program
test_suite_SOURCES = test_broker.c  # add all your test .c files here

# Compiler and linker flags from pkg-config
test_suite_CFLAGS  = $(CUNIT_CFLAGS)
test_suite_LDADD   = $(CUNIT_LIBS)

# If your test needs internal library code, link it, e.g.:
# test_suite_LDADD += $(top_builddir)/src/main/libcmom.a

# Tell Automake to run these programs during "make check"
TESTS = $(check_PROGRAMS)

# Optional: enable verbose test output
# AM_CFLAGS = -Wall -Wextra

endif