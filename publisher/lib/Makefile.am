#load common setting
include $(top_srcdir)/publisher/publisher_am.conf

lib_LTLIBRARIES = libpublisher.la

libpublisher_la_SOURCES = publisher.c  publish_content.c
#for statik library
libpublisher_la_LDFLAGS=-shared  $(DPUTIL_DIR)/lib/libdputil.la
libpublisher_la_LDFLAGS+=-Wl,--version-script,publisher.map
