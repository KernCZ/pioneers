# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
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

noinst_LIBRARIES += libpioneers.a

if HAVE_GNOME
include common/gtk/Makefile.am
endif

libpioneers_a_CPPFLAGS = $(console_cflags)

libpioneers_a_SOURCES = \
	common/authors.h \
	common/buildrec.c \
	common/buildrec.h \
	common/cards.c \
	common/cards.h \
	common/common_glib.c \
	common/common_glib.h \
	common/cost.c \
	common/cost.h \
	common/driver.c \
	common/driver.h \
	common/game.c \
	common/game.h \
	common/log.c \
	common/log.h \
	common/map.c \
	common/map.h \
	common/map_query.c \
	common/network.c \
	common/network.h \
	common/quoteinfo.c \
	common/quoteinfo.h \
	common/state.c \
	common/state.h

common/authors.h: AUTHORS
	@mkdir_p@ common
	echo -n "#define AUTHORLIST " > $@
	sed -e's/ <.*//; s/$$/", \\/; s/^/"/; /^"[[:space:]]*", \\$$/d' $< >> $@
	echo "NULL" >> $@

# This target is not called common/version.h (although it builds that file),
# because it must be PHONY, but should only be rebuilt once.
build_version:
	@mkdir_p@ common
	echo -n '#define FULL_VERSION "$(VERSION)' >> common/version.new
	if svn info > /dev/null 2>&1; then				\
		echo -n ".r`svn info | grep Revision | cut -f2 -d\ `"	\
			>> common/version.new				;\
		if svn status | grep -vq ^\? ; then			\
			echo -n '.M' >> common/version.new		;\
		fi							;\
	fi
	echo '"' >> common/version.new
	if diff common/version.h common/version.new > /dev/null 2>&1; then \
		rm common/version.new					;\
	else								\
		mv common/version.new common/version.h			;\
	fi

BUILT_SOURCES += common/authors.h build_version
CLEANFILES += common/authors.h common/version.h

# always try to rebuild version.h
.PHONY: build_version
