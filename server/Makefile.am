# Gnocatan - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 the Free Software Foundation
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

include $(top_srcdir)/rules.make

configdir=$(datadir)/games/gnocatan
Gamesdir=$(datadir)/gnome/apps/Games

bin_PROGRAMS = gnocatan-server-gtk gnocatan-server-console

noinst_LIBRARIES = libgnocatan_server.a

INCLUDES = \
	-I$(srcdir)/../common -I. \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATADIR=\"$(datadir)\" \
	-I$(includedir) \
	$(GNOME2_CFLAGS) \
	$(debug_includes)

gnocatan_server_gtk_DEPENDENCIES = libgnocatan_server.a \
	../common/libgnocatan.a ../common/libgnocatan_gtk.a

gnocatan_server_console_DEPENDENCIES = libgnocatan_server.a \
	../common/libgnocatan.a

libgnocatan_server_a_SOURCES = \
	buildutil.c \
	develop.c \
	discard.c \
	gold.c \
	gnocatan-server.c \
	gnocatan-server.h \
	meta.c \
	player.c \
	pregame.c \
	resource.c \
	robber.c \
	server.c \
	server.h \
	trade.c \
	turn.c

gnocatan_server_gtk_SOURCES = \
	gnocatan-server-gtk.c

gnocatan_server_console_SOURCES = \
	gnocatan-server-console.c \
	glib-driver.c \
	glib-driver.h

gnocatan_server_gtk_LDADD = \
	libgnocatan_server.a \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
	$(GNOME2_LIBS)

gnocatan_server_console_LDADD = \
	libgnocatan_server.a \
	../common/libgnocatan.a \
	$(GLIB2_LIBS)

config_DATA = \
	default.game \
	5-6-player.game \
	four-islands.game \
	seafarers.game \
	seafarers-gold.game \
	small.game \
	canyon.game \
	conquest.game \
	conquest+ports.game \
	pond.game \
	square.game \
	star.game \
	x.game \
	Cube.game \
	Another_swimming_pool_in_the_wall.game \
	Evil_square.game \
	GuerreDe100ans.game \
	Mini_another_swimming_pool_in_the_wall.game \
	henjes.game \
	lorindol.game

Games_DATA = gnocatan-server.desktop

EXTRA_DIST = \
	$(config_DATA) \
	$(Games_DATA)

# We don't need this, do we?  -- egnor
# install-data-local:
#	-chgrp games $(bindir)/gnocatan-server && chmod 2111 $(bindir)/gnocatan-server
