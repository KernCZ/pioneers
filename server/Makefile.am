configdir=$(datadir)/gnocatan

bin_PROGRAMS = gnocatan-server gnocatan-server-console

noinst_LIBRARIES = libgnocatan_server.a

INCLUDES = \
	-I$(srcdir)/../common -I. \
	$(GNOME_INCLUDEDIR)

gnocatan_server_DEPENDENCIES = libgnocatan_server.a \
	../common/libgnocatan.a ../common/libgnocatan_gtk.a

gnocatan_server_console_DEPENDENCIES = libgnocatan_server.a \
	../common/libgnocatan.a ../common/libgnocatan_gtk.a

libgnocatan_server_a_SOURCES = \
	buildutil.c \
	develop.c \
	discard.c \
	meta.c \
	player.c \
	pregame.c \
	resource.c \
	robber.c \
	server.c \
	server.h \
	trade.c \
	turn.c

gnocatan_server_SOURCES = \
	gnocatan-server.c

gnocatan_server_console_SOURCES = \
	gnocatan-server-console.c

gnocatan_server_LDADD = \
	libgnocatan_server.a \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)

gnocatan_server_console_LDADD = \
	libgnocatan_server.a \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
	$(GLIB_LIBS)

config_DATA = \
	default.game \
	four-islands.game \
	seafarers.game \
	small.game

EXTRA_DIST = \
	$(config_DATA)

# We don't need this, do we?  -- egnor
# install-data-local:
#	-chgrp games $(bindir)/gnocatan-server && chmod 2111 $(bindir)/gnocatan-server
