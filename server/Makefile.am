configdir=$(datadir)/gnocatan

bin_PROGRAMS = gnocatan-server

INCLUDES = \
	-I$(srcdir)/../common -I. \
	$(GNOME_INCLUDEDIR)

gnocatan_server_DEPENDENCIES = ../common/libgnocatan.a \
	../common/libgnocatan_gtk.a

gnocatan_server_SOURCES = \
	buildutil.c \
	develop.c \
	discard.c \
	gnocatan-server.c \
	meta.c \
	player.c \
	pregame.c \
	resource.c \
	robber.c \
	server.c \
	server.h \
	trade.c \
	turn.c

gnocatan_server_LDADD = \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)

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
