# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

configdir=$(datadir)/games/pioneers

DIST_SUBDIRS = gtk

if BUILD_SERVER

if HAVE_GNOME
SUBDIRS = . gtk
endif

bin_PROGRAMS = pioneers-server-console

noinst_LIBRARIES = libpioneers_server.a

AM_CPPFLAGS = \
	-I$(srcdir)/../common -I. \
	-I$(includedir) \
	$(GLIB2_CFLAGS)

pioneers_server_console_DEPENDENCIES = libpioneers_server.a \
	../common/libpioneers.a

libpioneers_server_a_SOURCES = \
	admin.c \
	admin.h \
	buildutil.c \
	develop.c \
	discard.c \
	gold.c \
	meta.c \
	player.c \
	pregame.c \
	resource.c \
	robber.c \
	server.c \
	server.h \
	trade.c \
	turn.c

pioneers_server_console_SOURCES = \
	main.c \
	glib-driver.c \
	glib-driver.h

pioneers_server_console_LDADD = \
	libpioneers_server.a \
	../common/libpioneers.a \
	$(GLIB2_LIBS)

endif # BUILD_SERVER

config_DATA = \
	default.game \
	5-6-player.game \
	four-islands.game \
	seafarers.game \
	seafarers-gold.game \
	small.game \
	archipel_gold.game \
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
