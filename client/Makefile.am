imagedir  =$(datadir)/games/gnocatan/images
appicondir=$(datadir)/pixmaps
pixmapdir =$(datadir)/pixmaps/gnocatan
Gamesdir=$(datadir)/gnome/apps/Games

CPPFLAGS += -DIMAGEDIR=\"$(imagedir)\"

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
	authors.h \
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
	i18n.c \
	i18n.h \
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
	theme.c \
	theme.h \
	trade.c \
	turn.c

gnocatan_LDADD = \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)

appicon_DATA = gnome-gnocatan.png

image_DATA = \
	board.png \
	cross.png \
	desert.png \
	field.png \
	forest.png \
	hill.png \
	mountain.png \
	pasture.png \
	plain.png \
	sea.png \
	splash.png \
	tick.png

pixmap_DATA = \
	bridge.png \
	city.png \
	develop.png \
	dice.png \
	finish.png \
	road.png \
	settlement.png \
	ship.png \
	trade.png

Games_DATA = gnocatan.desktop

EXTRA_DIST = \
	$(appicon_DATA) \
	$(image_DATA) \
	$(pixmap_DATA) \
	$(Games_DATA)

dist-hook:
	tar -c -f - themes | tar -x -f - -C $(distdir)
	find $(distdir)/themes -name CVS | xargs rm -rf

install-data-hook:
	mkdir -p $(DESTDIR)$(imagedir)
	tar -c -f - -C themes . | \
	tar -x -f - --mode=u+w,ugo+rX --no-same-owner -C $(DESTDIR)$(imagedir)
	find $(DESTDIR)$(imagedir) -name CVS | xargs rm -rf

gui.o:	authors.h

authors.h: ../AUTHORS
	echo -n "#define AUTHORLIST " > authors.h
	sed -e's/ <.*//; s/$$/", \\/; s/^/"/; /^"[[:space:]]*", \\$$/d' ../AUTHORS >> authors.h
	echo "NULL" >> authors.h
