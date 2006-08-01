# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
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

# some settings
console_cflags = \
	-I$(top_srcdir)/common \
	-I$(top_builddir)/common \
	-I$(includedir) \
	$(GLIB2_CFLAGS) \
	$(WARNINGS) \
	$(DEBUGGING) \
	$(GLIB_DEPRECATION) \
	-DDATADIR=\""$(pioneers_datadir)"\" \
	-DTHEMEDIR=\""$(pioneers_themedir)"\" \
	-DLOCALEDIR=\""$(pioneers_localedir)"\" \
	-DPIONEERS_DIR_DEFAULT=\""$(pioneers_datadir)/games/pioneers"\" \
	-DPIONEERS_SERVER_CONSOLE_PATH=\""$(bindir)/pioneers-server-console"\" \
	-DPIONEERS_SERVER_GTK_PATH=\""$(bindir)/pioneers-server-gtk"\" \
	-DPIONEERS_AI_PATH=\""$(bindir)/pioneersai"\"

gtk_cflags = \
	$(console_cflags) \
	-I$(top_srcdir)/common/gtk/ \
	$(GNOME2_CFLAGS) \
	$(GTK2_CFLAGS) \
	$(GTK_DEPRECATION)

# The Fink port needs an explicit reference to driver.o
console_libs = \
	libpioneers.a \
	$(top_builddir)/common/libpioneers_a-driver.o \
	$(GLIB2_LIBS)

gtk_libs = \
	$(console_libs) \
	libpioneers_gtk.a \
	$(GNOME2_LIBS) \
	$(GTK2_LIBS)

configdir = $(datadir)/games/pioneers
icondir = $(datadir)/pixmaps
pixmapdir = $(datadir)/pixmaps/pioneers
desktopdir = $(datadir)/applications
themedir = $(pioneers_themedir)

# Let object files be generated in their own subdirectories
AUTOMAKE_OPTIONS = subdir-objects

# set up these variables so the included Makefile.ams can use +=
SUBDIRS =
bin_PROGRAMS =
noinst_PROGRAMS =
noinst_LIBRARIES =
man_MANS =
config_DATA =
icon_DATA =
pixmap_DATA =
desktop_DATA =
theme_DATA =
CLEANFILES =
MAINTAINERCLEANFILES =
EXTRA_DIST = autogen.sh pioneers.spec xmldocs.make omf.make README.Cygwin README.MinGW
BUILT_SOURCES =
windows_icons =

# creating icons
%.png: %.svg
	@mkdir_p@ $(dir $@)
	$(svg_renderer_path) $(svg_renderer_width)48$(svg_renderer_height)48 $< $@

%.ico: %.svg
	@mkdir_p@ $(dir $@)
	$(svg_renderer_path) $(svg_renderer_width)16$(svg_renderer_height)16 $< $@-p16.png
	$(pngtopnm) $@-p16.png > $@-p16.pnm
	pngtopnm -alpha $@-p16.png > $@-p16a.pnm
	pnmcolormap 256 $@-p16.pnm > $@-p16colormap.pnm
	pnmremap -mapfile=$@-p16colormap.pnm $@-p16.pnm > $@-p16256.pnm
	$(svg_renderer_path) $(svg_renderer_width)32$(svg_renderer_height)32 $< $@-p32.png
	pngtopnm $@-p32.png > $@-p32.pnm
	pngtopnm -alpha $@-p32.png > $@-p32a.pnm
	pnmcolormap 256 $@-p32.pnm > $@-p32colormap.pnm
	pnmremap -mapfile=$@-p32colormap.pnm $@-p32.pnm > $@-p32256.pnm
	ppmtowinicon -andpgms $@-p32256.pnm $@-p32a.pnm $@-p16256.pnm $@-p16a.pnm > $@
	rm $@-p16.png
	rm $@-p16.pnm
	rm $@-p16a.pnm
	rm $@-p16colormap.pnm
	rm $@-p16256.pnm
	rm $@-p32.png
	rm $@-p32.pnm
	rm $@-p32a.pnm
	rm $@-p32colormap.pnm
	rm $@-p32256.pnm

if WINDOWS_ICON
# Will be used in Windows builds
%-icon.o: %.rc
	@mkdir_p@ $(dir $@)
	windres -I$(top_srcdir) -o $@ $<
endif

if BUILD_CLIENT
include client/Makefile.am
endif

#if BUILD_SERVER
include server/Makefile.am
#endif

if BUILD_META_SERVER
include meta-server/Makefile.am
endif

if BUILD_EDITOR
include editor/Makefile.am
endif

include common/Makefile.am
include docs/Makefile.am
include macros/Makefile.am

# Make use of some of the variables that were filled in by the included
# Makefile.ams
MAINTAINERCLEANFILES += $(icon_DATA) $(windows_icons)
EXTRA_DIST += \
	$(man_MANS) \
	$(desktop_DATA) \
	$(config_DATA) \
	$(theme_DATA) \
	$(pixmap_DATA) \
	$(icon_DATA) \
	$(subst png,svg,$(icon_DATA)) \
	$(windows_icons)

# po doesn't use automake, but creates its own Makefile
SUBDIRS += po

distclean-local:
	rm -f *~
	rm -rf autom4te.cache

# This line is added, because scrollkeeper leaves many files 
# in /var/scrollkeeper, that makes 'make distcheck' fail
distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper'

# Reformat the code.
reindent:
	find . -name '*.[ch]' -exec indent -kr -i8 '{}' ';'

restorepo:
	svn revert po/*.po po/pioneers.pot
