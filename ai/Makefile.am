configdir=$(datadir)/games/gnocatan

bin_PROGRAMS = gnocatanai

gnocatanai_DEPENDENCIES = ../common/libgnocatan.a ../common/libgnocatan_gtk.a

INCLUDES = \
	-I$(srcdir)/../common -I. \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(ADMIN_GTK) \
        -I$(includedir) \
	$(GNOME2_CFLAGS)

gnocatanai_SOURCES = \
	ai.c \
	client.c \
	client.h \
	greedy.c \
	greedy.h \
	computer.h \
	computer.c \
	develop.c \
	discard.c \
	monopoly.c \
	player.c \
	player.h \
	plenty.c \
	resource.c \
	robber.c \
	stock.c \
	trade.c \
	turn.c

gnocatanai_LDADD = \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
	$(GLIB2_LIBS)

config_DATA = \
	computer_names

EXTRA_DIST = \
	$(config_DATA)
