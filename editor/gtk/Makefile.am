appicondir=$(datadir)/pixmaps
configdir=$(datadir)/games/pioneers
desktopdir=$(datadir)/applications

appicon_DATA = pioneers-editor.png

noinst_DATA = pioneers-editor.svg

bin_PROGRAMS = pioneers-editor

pioneers_editor_DEPENDENCIES = \
	../../common/libpioneers.a \
	../../common/gtk/libpioneers_gtk.a

AM_CPPFLAGS = \
	-I$(srcdir)/../../common -I. \
	-I$(srcdir)/../../common/gtk -I. \
	-I$(includedir) \
	$(GNOME2_CFLAGS) \
	$(GTK2_CFLAGS)

pioneers_editor_SOURCES = \
	editor.c \
	game-parameters.c \
	game-parameters.h \
	game-devcards.c \
	game-devcards.h \
	game-buildings.c \
	game-buildings.h \
	game-resources.c \
	game-resources.h

pioneers_editor_LDADD = \
	../../common/libpioneers.a \
	../../common/gtk/libpioneers_gtk.a \
	$(GNOME2_LIBS) \
	$(GTK2_LIBS)

desktop_DATA = pioneers-editor.desktop

EXTRA_DIST = \
	$(appicon_DATA) \
	$(desktop_DATA) \
	$(noinst_DATA)

MAINTAINERCLEANFILES = $(appicon_DATA)

pioneers-editor.png: pioneers-editor.svg
	rsvg --width 48 $< $@

