if HAVE_CUNIT

AM_CFLAGS = -g -O0
check_PROGRAMS = test_suite
test_suite_SOURCES = test_stomp.c
test_suite_CFLAGS = -I$(top_srcdir)/includes $(CUNIT_CFLAGS)

# Option A: Reorder libraries (simplest if no circular deps)
test_suite_LDADD = $(top_builddir)/src/main/libbroker-protocols.a \
                   $(top_builddir)/src/main/libbroker-core.a \
                   $(LOG4C_LIBS) \
                   $(CUNIT_LIBS)

TESTS = $(check_PROGRAMS)

endif