pixmapdir = $(datadir)/pixmaps/gnocatan
appicondir=$(datadir)/pixmaps
Gamesdir=$(datadir)/gnome/apps/Games

SUBDIRS = help

bin_PROGRAMS = gnocatan

gnocatan_DEPENDENCIES = ../common/libgnocatan.a ../common/libgnocatan_gtk.a

# if anyone knows a cleaner way to do this, be my guest.  Automake screamed
#   at me when I tried to do it more directly.
if ADMIN_GTK_SUPPORT
ADMIN_GTK = -DADMIN_GTK
ADMIN_GTK_FILES_ACTIVE = admin-gtk.c
ADMIN_GTK_FILES_INACTIVE =
else
ADMIN_GTK =
ADMIN_GTK_FILES_ACTIVE =
ADMIN_GTK_FILES_INACTIVE = admin-gtk.c
endif

INCLUDES = \
	-I$(srcdir)/../common -I. \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(ADMIN_GTK) \
        -I$(includedir) $(GNOME_INCLUDEDIR)

EXTRA_gnocatan_SOURCES = $(ADMIN_GTK_FILES_INACTIVE)

gnocatan_SOURCES = \
	$(ADMIN_GTK_FILES_ACTIVE) \
	build.c \
	client.c \
	client.h \
	chat.c \
	connect.c \
	config-gnome.c \
	config-gnome.h \
	develop.c \
	discard.c \
	gameover.c \
	gnocatan.c \
	gui.c \
	gui.h \
	guimap.c \
	guimap.h \
	histogram.c \
	histogram.h \
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
	settingscreen.c \
	stock.c \
	trade.c \
	turn.c

gnocatan_LDADD = \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
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
	splash.png \
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
