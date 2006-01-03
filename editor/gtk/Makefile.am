appicondir=$(datadir)/pixmaps
configdir=$(datadir)/games/pioneers
desktopdir=$(datadir)/applications

appicon_DATA = pioneers-editor.png

noinst_DATA = \
	pioneers-editor.svg \
	pioneers-editor.ico

bin_PROGRAMS = pioneers-editor

pioneers_editor_DEPENDENCIES = \
	../../common/libpioneers.a \
	../../common/gtk/libpioneers_gtk.a \
	$(WINDOWS_ICON)

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
	game-resources.h \
	pioneers-editor.rc

pioneers_editor_LDADD = \
	../../common/libpioneers.a \
	../../common/gtk/libpioneers_gtk.a \
	$(GNOME2_LIBS) \
	$(GTK2_LIBS) \
	$(WINDOWS_ICON)

desktop_DATA = pioneers-editor.desktop

EXTRA_DIST = \
	$(appicon_DATA) \
	$(desktop_DATA) \
	$(noinst_DATA)

MAINTAINERCLEANFILES = $(appicon_DATA)

pioneers-editor.png: pioneers-editor.svg
	$(svg_renderer_path) $(svg_renderer_width)48$(svg_renderer_height)48 $< $@

# Will be used in Windows builds
$(WINDOWS_ICON): pioneers-editor.rc
	windres -o $(WINDOWS_ICON) pioneers-editor.rc
