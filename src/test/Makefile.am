noinst_LIBRARIES = libtestfixtures.a
libtestfixtures_a_SOURCES = fixtures/files.c \
                            asserts/files.c
libtestfixtures_a_CFLAGS = -g -O0 $(AM_CFLAGS)

TESTS = check_stomp check_storage
check_PROGRAMS = check_stomp check_storage

check_stomp_SOURCES = stomp.c
check_stomp_CFLAGS = -g -O0 -I$(top_srcdir)/includes -I$(top_srcdir)/src/test/fixtures @CHECK_CFLAGS@

check_stomp_LDADD = $(top_builddir)/src/main/libbroker-protocols.a \
                    $(top_builddir)/src/main/libbroker-core.a \
                    $(LOG4C_LIBS) \
                    @CHECK_LIBS@ \
                    ./libtestfixtures.a

check_storage_SOURCES = storage.c
check_storage_CFLAGS = -g -O0 -I$(top_srcdir)/includes -I$(top_srcdir)/src/test/fixtures @CHECK_CFLAGS@
check_storage_LDADD = $(top_builddir)/src/main/libbroker-protocols.a \
                      $(top_builddir)/src/main/libbroker-core.a \
                      $(top_builddir)/src/main/libbroker-storage.a \
                      $(LOG4C_LIBS) \
                      @CHECK_LIBS@ \
                      ./libtestfixtures.a