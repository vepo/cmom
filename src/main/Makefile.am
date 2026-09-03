noinst_LIBRARIES = libbroker-core.a libbroker-protocols.a

libbroker_core_a_SOURCES = core/logger.c \
                           coroutines.c \
                           async_io.c \
                           transport.c \
                           event_watch.c
libbroker_core_a_CFLAGS = -I$(top_srcdir)/includes $(LOG4C_CFLAGS) $(AM_CFLAGS)

libbroker_protocols_a_SOURCES = protocols/protocol.c \
                                protocols/stomp.c
libbroker_protocols_a_CFLAGS = -I$(top_srcdir)/includes $(LOG4C_CFLAGS) $(AM_CFLAGS)

bin_PROGRAMS = broker
broker_SOURCES = broker.c
broker_CFLAGS  = -I$(top_srcdir)/includes $(LOG4C_CFLAGS) $(AM_CFLAGS)
broker_LDFLAGS = -Wl,--start-group -Wl,--end-group
broker_LDADD   = libbroker-core.a libbroker-protocols.a $(LOG4C_LIBS) $(LDADD)