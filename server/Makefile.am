configdir=$(datadir)/gnocatan

bin_PROGRAMS = gnocatan-server-gtk gnocatan-server-console

noinst_LIBRARIES = libgnocatan_server.a

INCLUDES = \
	-I$(srcdir)/../common -I. \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir) $(GNOME_INCLUDEDIR)

gnocatan_server_gtk_DEPENDENCIES = libgnocatan_server.a \
	../common/libgnocatan.a ../common/libgnocatan_gtk.a

gnocatan_server_console_DEPENDENCIES = libgnocatan_server.a \
	../common/libgnocatan.a

libgnocatan_server_a_SOURCES = \
	buildutil.c \
	develop.c \
	discard.c \
	gnocatan-server.c \
	gnocatan-server.h \
	meta.c \
	player.c \
	pregame.c \
	resource.c \
	robber.c \
	server.c \
	server.h \
	trade.c \
	turn.c

gnocatan_server_gtk_SOURCES = \
	gnocatan-server-gtk.c

gnocatan_server_console_SOURCES = \
	gnocatan-server-console.c \
	glib-driver.c \
	glib-driver.h

gnocatan_server_gtk_LDADD = \
	libgnocatan_server.a \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)

gnocatan_server_console_LDADD = \
	libgnocatan_server.a \
	../common/libgnocatan.a \
	$(GLIB_LIBS)

config_DATA = \
	5-6-player.game \
	default.game \
	four-islands.game \
	seafarers.game \
	small.game \
	canyon.game \
	conquest.game \
	conquest+ports.game \
	pond.game \
	square.game \
	star.game \
	x.game

EXTRA_DIST = \
	$(config_DATA)

# We don't need this, do we?  -- egnor
# install-data-local:
#	-chgrp games $(bindir)/gnocatan-server && chmod 2111 $(bindir)/gnocatan-server
