bin_PROGRAMS = pioneers-meta-server

AM_CPPFLAGS = \
	-I$(srcdir)/../common -I. \
	-I$(includedir) \
	$(GLIB2_CFLAGS)

pioneers_meta_server_SOURCES = \
	main.c

# Some linkers need an additional include of driver.o
pioneers_meta_server_LDADD = \
	../common/libpioneers.a \
	../common/driver.o \
	$(GLIB2_LIBS)
