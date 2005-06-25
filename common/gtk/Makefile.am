# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 the Free Software Foundation
# Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
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

noinst_LIBRARIES = libpioneers_gtk.a

themedir  =$(datadir)/games/gnocatan/themes

AM_CPPFLAGS = \
	-I$(includedir) \
	-I$(srcdir)/.. \
	-DDATADIR=\"$(datadir)\" \
	-DTHEMEDIR=\"$(themedir)\" \
	-I$(GNOME2_CFLAGS) \
	$(debug_includes) \
	$(extra_includes)

libpioneers_gtk_a_SOURCES = \
	aboutbox.c \
	aboutbox.h \
	colors.c \
	colors.h \
	common_gtk.c \
	common_gtk.h \
	config-gnome.c \
	config-gnome.h \
	game-settings.c \
	game-settings.h \
	guimap.c \
	guimap.h \
	polygon.c \
	polygon.h \
	select-game.c \
	select-game.h \
	theme.c \
	theme.h
