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

if HAVE_GNOME
SUBDIRS = . gtk
endif

DIST_SUBDIRS = gtk

bin_PROGRAMS = gnocatan-server-console

noinst_LIBRARIES = libgnocatan_server.a

AM_CPPFLAGS = \
	-I$(srcdir)/../common -I. \
	-DDATADIR=\"$(datadir)\" \
	-I$(includedir) \
	$(debug_includes) \
	$(extra_includes)

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

gnocatan_server_console_SOURCES = \
	gnocatan-server-console.c \
	glib-driver.c \
	glib-driver.h

gnocatan_server_console_LDADD = \
	libgnocatan_server.a \
	../common/libgnocatan.a 

config_DATA = \
	default.game \
	5-6-player.game \
	four-islands.game \
	seafarers.game \
	seafarers-gold.game \
	small.game \
	canyon.game \
	coeur.game \
	conquest.game \
	conquest+ports.game \
	crane_island.game \
	iles.game \
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

EXTRA_DIST = \
	$(config_DATA)
