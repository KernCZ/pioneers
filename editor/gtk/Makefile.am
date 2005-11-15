configdir=$(datadir)/games/pioneers

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
