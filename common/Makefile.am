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

noinst_LIBRARIES = libgnocatan.a libgnocatan_gtk.a

noinst_HEADERS = authors.h

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
        -I$(includedir) $(GNOME2_CFLAGS) \
	$(debug_includes)

libgnocatan_a_SOURCES = \
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
	gnocatan-path.h.in \
	log.c \
	log.h \
	map.c \
	map.h \
	map_query.c \
	meta.h \
	network.c \
	network.h \
	quoteinfo.c \
	quoteinfo.h \
	state.c \
	state.h

EXTRA_libgnocatan_a_SOURCES = \
	mt_rand.c \
	mt_rand.h

libgnocatan_a_LIBADD = \
	$(MT_RAND_OBJ)

libgnocatan_a_DEPENDENCIES = \
	$(MT_RAND_OBJ) gnocatan-path.h

libgnocatan_gtk_a_SOURCES = \
	common_gtk.c \
	common_gtk.h \
	config-gnome.c \
	config-gnome.h

gnocatan-path.h: gnocatan-path.h.in
	rm -f $(<D)/$(@F)
	ddir="`echo $(datadir)`"; \
	bdir="`echo $(bindir)`"; \
	sed -e "s,@datadir\@,$$ddir,g" -e "s,@bindir\@,$$bdir,g" $< >$(<D)/$(@F)

authors.h: ../AUTHORS
	echo -n "#define AUTHORLIST " > $@
	sed -e's/ <.*//; s/$$/", \\/; s/^/"/; /^"[[:space:]]*", \\$$/d' $< >> $@
	echo "NULL" >> $@
