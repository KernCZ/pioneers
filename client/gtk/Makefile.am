# Gnocatan - Implementation of the excellent Settlers of Catan board game.
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

SUBDIRS = data

themedir  =$(datadir)/games/gnocatan/themes
appicondir=$(datadir)/pixmaps
pixmapdir =$(datadir)/pixmaps/gnocatan
Gamesdir=$(datadir)/gnome/apps/Games

bin_PROGRAMS = gnocatan

gnocatan_DEPENDENCIES = \
	../common/libgnocatanclient.a \
	../../common/gtk/libgnocatan_gtk.a \
	../../common/libgnocatan.a

# if anyone knows a cleaner way to do this, be my guest.  Automake screamed
#   at me when I tried to do it more directly.
if ADMIN_GTK_SUPPORT
ADMIN_GTK = -DADMIN_GTK
ADMIN_GTK_FILES_ACTIVE = admin-gtk.c
ADMIN_GTK_FILES_INACTIVE =
else
ADMIN_GTK =
ADMIN_GTK_FILES_ACTIVE =
ADMIN_GTK_FILES_INACTIVE = admin-gtk.c
endif

AM_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../.. \
	-I$(srcdir)/../../common \
	-I$(srcdir)/../../common/gtk \
	-DTHEMEDIR=\"$(themedir)\" \
	-DDATADIR=\"$(datadir)\" \
	$(ADMIN_GTK) \
        -I$(includedir) \
	$(GNOME2_CFLAGS)  \
	$(debug_includes) \
	$(extra_includes)

EXTRA_gnocatan_SOURCES = $(ADMIN_GTK_FILES_INACTIVE)

gnocatan_SOURCES = \
	$(ADMIN_GTK_FILES_ACTIVE) \
	frontend.h \
	gui.h \
	guimap.h \
	histogram.h \
	polygon.h \
	theme.h \
	callbacks.c \
	chat.c \
	connect.c \
	develop.c \
	discard.c \
	frontend.c \
	gameover.c \
	gold.c \
	gui.c \
	guimap.c \
	histogram.c \
	identity.c \
	interface.c \
	legend.c \
	monopoly.c \
	name.c \
	offline.c \
	plenty.c \
	polygon.c \
	player.c \
	quote.c \
	resource.c \
	resource-table.c \
	resource-table.h \
	settingscreen.c \
	state.c \
	theme.c \
	trade.c

gnocatan_LDADD = \
	../common/libgnocatanclient.a \
	../../common/libgnocatan.a \
	../../common/gtk/libgnocatan_gtk.a \
	$(GNOME2_LIBS)
