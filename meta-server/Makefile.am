
include $(top_srcdir)/rules.make

bin_PROGRAMS = gnocatan-meta-server

AM_CPPFLAGS = \
	-I$(srcdir)/../common -I. \
	-I$(includedir) \
	$(debug_includes) \
	$(extra_includes)

gnocatan_meta_server_SOURCES = \
	gnocatan-meta-server.c
