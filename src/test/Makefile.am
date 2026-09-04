AM_CPPFLAGS = -I$(top_srcdir)/includes 

TESTS = check_broker
check_PROGRAMS = check_broker

check_broker_SOURCES = stomp.c

check_broker_CFLAGS = @CHECK_CFLAGS@
check_broker_LDADD = $(top_builddir)/src/main/libbroker-protocols.a \
                     $(top_builddir)/src/main/libbroker-core.a \
                     $(LOG4C_LIBS) \
                     @CHECK_LIBS@