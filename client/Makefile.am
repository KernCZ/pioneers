pixmapdir = $(datadir)/pixmaps/gnocatan
appicondir=$(datadir)/pixmaps
Gamesdir=$(datadir)/gnome/apps/Games

SUBDIRS = help

bin_PROGRAMS = gnocatan

INCLUDES = \
	-I$(srcdir)/../common -I. \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
        -I$(includedir) $(GNOME_INCLUDEDIR)

gnocatan_DEPENDENCIES = ../common/libgnocatan.a ../common/libgnocatan_gui.a

gnocatan_SOURCES = \
	build.c \
	client.c \
	client.h \
	chat.c \
	connect.c \
	develop.c \
	discard.c \
	gameover.c \
	gnocatan.c \
	gui.c \
	gui.h \
	guimap.c \
	guimap.h \
	identity.c \
	legend.c \
	monopoly.c \
	name.c \
	player.c \
	player.h \
	plenty.c \
	polygon.c \
	polygon.h \
	quote.c \
	resource.c \
	road_building.c \
	robber.c \
	setup.c \
	stock.c \
	trade.c \
	turn.c

gnocatan_LDADD = \
	../common/libgnocatan.a \
	../common/libgnocatan_gui.a \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)

appicon_DATA = gnome-gnocatan.png

pixmap_DATA = \
	board.png \
	bridge.png \
	city.png \
	cross.png \
	desert.png \
	develop.png \
	dice.png \
	field.png \
	finish.png \
	forest.png \
	hill.png \
	mountain.png \
	pasture.png \
	plain.png \
	road.png \
	sea.png \
	settlement.png \
	ship.png \
	tick.png \
	trade.png

Games_DATA = gnocatan.desktop

EXTRA_DIST = \
	$(appicon_DATA) \
	$(pixmap_DATA) \
	$(Games_DATA)

# We don't need this, do we?  -- egnor
# install-data-local:
#	-chgrp games $(bindir)/gnocatan && chmod 2111 $(bindir)/gnocatan
