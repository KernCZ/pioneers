configdir=$(datadir)/games/gnocatan

bin_PROGRAMS = gnocatan-editor

include $(top_srcdir)/rules.make

gnocatan_editor_DEPENDENCIES = \
	../../common/libgnocatan.a \
	../../common/gtk/libgnocatan_gtk.a

AM_CPPFLAGS = \
	-I$(srcdir)/../../common -I. \
	-I$(srcdir)/../../common/gtk -I. \
	-DDATADIR=\"$(datadir)\" \
	$(ADMIN_GTK) \
	-I$(includedir) \
	$(GNOME2_CFLAGS) \
	$(debug_includes) \
	$(extra_includes)

gnocatan_editor_SOURCES = \
	editor.c \
	game-parameters.c \
	game-parameters.h \
	game-devcards.c \
	game-devcards.h \
	game-buildings.c \
	game-buildings.h \
	game-resources.c \
	game-resources.h \
	guimap.c \
	guimap.h

gnocatan_editor_LDADD = \
	../../common/libgnocatan.a \
	../../common/gtk/libgnocatan_gtk.a \
	$(GNOME2_LIBS)
