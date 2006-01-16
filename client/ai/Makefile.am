# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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

configdir = $(datadir)/games/pioneers

bin_PROGRAMS = pioneersai

pioneersai_DEPENDENCIES = \
	../common/libpioneersclient.a \
	../../common/libpioneers.a 

AM_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../.. \
	-I$(srcdir)/../../common \
        -I$(includedir) \
	$(GLIB2_CFLAGS)

pioneersai_SOURCES = \
	ai.h \
	ai.c \
	greedy.c

pioneersai_LDADD = \
	../common/libpioneersclient.a \
	../../common/libpioneers.a \
	$(GLIB2_LIBS)

config_DATA = \
	computer_names

EXTRA_DIST = \
	$(config_DATA)
