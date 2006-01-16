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

appicondir=$(datadir)/pixmaps
desktopdir=$(datadir)/applications

appicon_DATA = pioneers-server.png

noinst_DATA = pioneers-server.svg

bin_PROGRAMS = pioneers-server-gtk

AM_CPPFLAGS = \
	-I $(srcdir)/../../common \
	-I $(srcdir)/../../common/gtk \
	-I $(srcdir)/.. \
	-I$(includedir) \
	$(GNOME2_CFLAGS) \
	$(GTK2_CFLAGS)

pioneers_server_gtk_DEPENDENCIES = \
	../libpioneers_server.a \
	../../common/libpioneers.a \
	../../common/gtk/libpioneers_gtk.a

pioneers_server_gtk_SOURCES = \
	main.c

pioneers_server_gtk_LDADD = \
	../libpioneers_server.a \
	../../common/libpioneers.a \
	../../common/gtk/libpioneers_gtk.a \
	$(GNOME2_LIBS) \
	$(GTK2_LIBS)

desktop_DATA = pioneers-server.desktop

EXTRA_DIST = \
	$(appicon_DATA) \
	$(desktop_DATA) \
	$(noinst_DATA)

MAINTAINERCLEANFILES = $(appicon_DATA)

pioneers-server.png: pioneers-server.svg
	$(svg_renderer_path) $(svg_renderer_width)48$(svg_renderer_height)48 $< $@
