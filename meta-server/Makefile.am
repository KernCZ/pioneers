
include $(top_srcdir)/rules.make

bin_PROGRAMS = pioneers-meta-server

AM_CPPFLAGS = \
	-I$(srcdir)/../common -I. \
	-I$(includedir) \
	$(debug_includes) \
	$(extra_includes)

pioneers_meta_server_SOURCES = \
	main.c

pioneers_meta_server_LDADD = \
	../common/libpioneers.a
