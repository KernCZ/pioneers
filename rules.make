# Gnocatan - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 2004 Bas Wijnen <b.wijnen@phys.rug.nl>
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

extra_includes = \
	-DLOCALEDIR=\""$(datadir)/locale"\" \
	-DGNOCATAN_DIR_DEFAULT=\""$(datadir)/games/gnocatan"\" \
	-DGNOCATAN_SERVER_CONSOLE_PATH=\""$(bindir)/gnocatan-server-console"\" \
	-DGNOCATAN_SERVER_GTK_PATH=\""$(bindir)/gnocatan-server-gtk"\" \
	-DGNOCATAN_AI_PATH=\""$(bindir)/gnocatanai"\"

if DEBUG
debug_includes = \
	-Wall \
	-W \
	-Wpointer-arith \
	-Wcast-qual \
	-Wwrite-strings \
	-Wno-sign-compare \
	-Waggregate-return \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wredundant-decls \
	-Wnested-externs \
	-ggdb3 \
	-O \
	-DGDK_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED \
	-DGNOME_DISABLE_DEPRECATED \
	-DDEBUG \
	-pedantic
else
debug_includes =
endif
