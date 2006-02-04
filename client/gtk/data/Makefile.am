# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
# Copyright (C) 2006 Roland Clobus <rclobus@bigfoot.com>
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

MAINTAINERCLEANFILES = $(appicon_DATA) pioneers.ico

pioneers.png: pioneers.svg
	$(svg_renderer_path) $(svg_renderer_width)48$(svg_renderer_height)48 $< $@

pioneers.ico: pioneers.svg
	$(svg_renderer_path) $(svg_renderer_width)16$(svg_renderer_height)16 $< p16.png
	$(pngtopnm) p16.png > p16.pnm
	pngtopnm -alpha p16.png > p16a.pnm
	pnmcolormap 256 p16.pnm > p16colormap.pnm
	pnmremap -mapfile=p16colormap.pnm p16.pnm > p16256.pnm
	$(svg_renderer_path) $(svg_renderer_width)32$(svg_renderer_height)32 $< p32.png
	pngtopnm p32.png > p32.pnm
	pngtopnm -alpha p32.png > p32a.pnm
	pnmcolormap 256 p32.pnm > p32colormap.pnm
	pnmremap -mapfile=p32colormap.pnm p32.pnm > p32256.pnm
	ppmtowinicon -andpgms p32256.pnm p32a.pnm p16256.pnm p16a.pnm > $@
	rm p16.png
	rm p16.pnm
	rm p16a.pnm
	rm p16colormap.pnm
	rm p16256.pnm
	rm p32.png
	rm p32.pnm
	rm p32a.pnm
	rm p32colormap.pnm
	rm p32256.pnm
