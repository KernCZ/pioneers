configdir=$(datadir)/games/pioneers

bin_PROGRAMS = pioneers-editor

include $(top_srcdir)/rules.make

pioneers_editor_DEPENDENCIES = \
	../../common/libpioneers.a \
	../../common/gtk/libpioneers_gtk.a

AM_CPPFLAGS = \
	-I$(srcdir)/../../common -I. \
	-I$(srcdir)/../../common/gtk -I. \
	-DDATADIR=\"$(datadir)\" \
	$(ADMIN_GTK) \
	-I$(includedir) \
	$(GNOME2_CFLAGS) \
	$(debug_includes) \
	$(extra_includes)

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
	$(GNOME2_LIBS)
