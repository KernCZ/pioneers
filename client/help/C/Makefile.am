# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
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

# see http://developer.gnome.org/dotplan/porting/ar01s06.html

figdir = images
docname = pioneers
lang = C
omffile = pioneers-C.omf
entities = legal.xml
include $(top_srcdir)/xmldocs.make
dist-hook: app-dist-hook

CLEANFILES += *.html pioneers.proc yelp-note*.png

# HTMLHelp is the compressed help format for Microsoft Windows
htmlhelp: pioneers.xml
	xmlto -m custom.xsl htmlhelp pioneers.xml
	-/cygdrive/c/Program\ Files/HTML\ Help\ Workshop/hhc.exe htmlhelp.hhp
	rm htmlhelp.hhp
	rm toc.hhc
	rm *.html

simplehtml: pioneers.xml
	xmlto -m custom.xsl html pioneers.xml

yelphtml: pioneers.xml
	xsltproc /usr/share/yelp-xsl/xslt/docbook/html/db2html.xsl pioneers.xml
	cp /usr/share/yelp-xsl/icons/hicolor/24x24/status/yelp-note*.png .

xmllint: pioneers.xml
	xmllint --format pioneers.xml -o pioneers.xml.good
	mv pioneers.xml.good pioneers.xml
