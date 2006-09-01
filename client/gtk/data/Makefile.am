# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
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

include client/gtk/data/themes/Makefile.am

icon_DATA += client/gtk/data/pioneers.png

theme_DATA += \
	client/gtk/data/cross.png \
	client/gtk/data/tick.png
	
pixmap_DATA += \
	client/gtk/data/bridge.png \
	client/gtk/data/city.png \
	client/gtk/data/develop.png \
	client/gtk/data/dice.png \
	client/gtk/data/finish.png \
	client/gtk/data/road.png \
	client/gtk/data/settlement.png \
	client/gtk/data/ship.png \
	client/gtk/data/ship_move.png \
	client/gtk/data/splash.png \
	client/gtk/data/trade.png \
	client/gtk/data/brick.png \
	client/gtk/data/grain.png \
	client/gtk/data/lumber.png \
	client/gtk/data/ore.png \
	client/gtk/data/wool.png

desktop_DATA += client/gtk/data/pioneers.desktop

if USE_WINDOWS_ICON
pioneers_LDADD += client/gtk/data/pioneers.res
endif

windows_resources_output += client/gtk/data/pioneers.res
windows_resources_output += client/gtk/data/pioneers.ico
windows_resources_input += client/gtk/data/pioneers.rc
