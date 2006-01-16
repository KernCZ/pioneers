# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
# Copyright (C) 2004 Roland Clobus <rclobus@bigfoot.com>
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

noinst_LIBRARIES = libpioneers.a 

if HAVE_GNOME
SUBDIRS = gtk
endif

DIST_SUBDIRS = gtk

noinst_HEADERS = authors.h

AM_CPPFLAGS = \
	-I$(includedir) \
	$(GLIB2_CFLAGS)

libpioneers_a_SOURCES = \
	buildrec.c \
	buildrec.h \
	cards.c \
	cards.h \
	common_glib.c \
	common_glib.h \
	cost.c \
	cost.h \
	driver.c \
	driver.h \
	game.c \
	game.h \
	log.c \
	log.h \
	map.c \
	map.h \
	map_query.c \
	network.c \
	network.h \
	quoteinfo.c \
	quoteinfo.h \
	state.c \
	state.h

authors.h: ../AUTHORS
	echo -n "#define AUTHORLIST " > $@
	sed -e's/ <.*//; s/$$/", \\/; s/^/"/; /^"[[:space:]]*", \\$$/d' $< >> $@
	echo "NULL" >> $@
