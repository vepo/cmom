if HAVE_CUNIT

check_PROGRAMS = test_stomp test_coroutines

test_stomp_SOURCES = test_stomp.c
test_stomp_CFLAGS  = -I$(top_srcdir)/include $(CUNIT_CFLAGS) $(AM_CFLAGS)
test_stomp_LDADD   = -Wl,--start-group $(top_builddir)/src/libcmom-core.a \
                     $(top_builddir)/src/libprotocols.a -Wl,--end-group \
                     $(CUNIT_LIBS) $(LOG4C_LIBS)

test_coroutines_SOURCES = test_coroutines.c
test_coroutines_CFLAGS  = -I$(top_srcdir)/include $(CUNIT_CFLAGS) $(AM_CFLAGS)
test_coroutines_LDADD   = -Wl,--start-group $(top_builddir)/src/libcmom-core.a \
                          $(top_builddir)/src/libprotocols.a -Wl,--end-group \
                          $(CUNIT_LIBS) $(LOG4C_LIBS)

TESTS = $(check_PROGRAMS)

endif