# This AI still depends on Gnome/Gtk.
# The ai in client/ai is the same ai as this, but is a true console application

configdir=$(datadir)/games/gnocatan

bin_PROGRAMS = gnocatanai

include $(top_srcdir)/rules.make

gnocatanai_DEPENDENCIES = ../common/libgnocatan.a ../common/libgnocatan_gtk.a

AM_CPPFLAGS = \
	-I$(srcdir)/../common -I. \
	$(ADMIN_GTK) \
	-I$(includedir) \
	$(GNOME2_CFLAGS) \
	$(debug_includes) \
	$(extra_includes)

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
	../common/libgnocatan_gtk.a 

config_DATA = \
	computer_names

EXTRA_DIST = \
	$(config_DATA)
