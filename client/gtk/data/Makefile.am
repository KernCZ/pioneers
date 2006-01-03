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

SUBDIRS = themes

themedir  =$(pioneers_themedir)
appicondir=$(datadir)/pixmaps
pixmapdir =$(datadir)/pixmaps/pioneers
desktopdir=$(datadir)/applications

appicon_DATA = pioneers.png

noinst_DATA = \
	pioneers.ico \
	pioneers.svg

theme_DATA = \
	cross.png \
	tick.png
	
pixmap_DATA = \
	bridge.png \
	city.png \
	develop.png \
	dice.png \
	finish.png \
	road.png \
	settlement.png \
	ship.png \
	ship_move.png \
	splash.png \
	trade.png

desktop_DATA = pioneers.desktop

EXTRA_DIST = \
	$(noinst_DATA) \
	$(appicon_DATA) \
	$(theme_DATA) \
	$(pixmap_DATA) \
	$(desktop_DATA)

MAINTAINERCLEANFILES = $(appicon_DATA)

pioneers.png: pioneers.svg
	$(svg_renderer_path) $(svg_renderer_width)48$(svg_renderer_height)48 $< $@
