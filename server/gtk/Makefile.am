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

bin_PROGRAMS = gnocatan-server-gtk

AM_CPPFLAGS = \
	-I $(srcdir)/../../common \
	-I $(srcdir)/../../common/gtk \
	-I $(srcdir)/.. \
	-DDATADIR=\"$(datadir)\" \
	-I$(includedir) \
	$(GNOME2_CFLAGS) \
	$(debug_includes) \
	$(extra_includes)

gnocatan_server_gtk_DEPENDENCIES = \
	../libgnocatan_server.a \
	../../common/libgnocatan.a \
	../../common/gtk/libgnocatan_gtk.a

gnocatan_server_gtk_SOURCES = \
	gnocatan-server-gtk.c

gnocatan_server_gtk_LDADD = \
	../libgnocatan_server.a \
	../../common/libgnocatan.a \
	../../common/gtk/libgnocatan_gtk.a \
	$(GNOME2_LIBS)

Games_DATA = gnocatan-server.desktop

EXTRA_DIST = \
	$(Games_DATA)
