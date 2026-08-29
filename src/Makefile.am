noinst_LIBRARIES = libcmom-core.a libprotocols.a

libcmom_core_a_SOURCES = coroutines.c \
                         async_io.c \
                         transport.c \
                         event_watch.c \
                         logger_global.c
libcmom_core_a_CFLAGS = -I$(top_srcdir)/include $(LOG4C_CFLAGS) $(AM_CFLAGS)

libprotocols_a_SOURCES = protocols/protocol.c \
                         protocols/stomp.c
libprotocols_a_CFLAGS = -I$(top_srcdir)/include $(LOG4C_CFLAGS) $(AM_CFLAGS)

bin_PROGRAMS = cmom
cmom_SOURCES = cmom.c
cmom_CFLAGS  = -I$(top_srcdir)/include $(LOG4C_CFLAGS) $(AM_CFLAGS)
cmom_LDADD   = -Wl,--start-group libcmom-core.a libprotocols.a -Wl,--end-group \
               $(LOG4C_LIBS) $(LDADD)