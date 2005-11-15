bin_PROGRAMS = pioneers-meta-server

AM_CPPFLAGS = \
	-I$(srcdir)/../common -I. \
	-I$(includedir) \
	$(GLIB2_CFLAGS)

pioneers_meta_server_SOURCES = \
	main.c

pioneers_meta_server_LDADD = \
	../common/libpioneers.a \
	$(GLIB2_LIBS)
