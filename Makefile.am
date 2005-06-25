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

include $(top_srcdir)/rules.make

SUBDIRS = po common client server meta-server editor docs

EXTRA_DIST = autogen.sh pioneers.spec xmldocs.make omf.make rules.make

dist-hook:
	tar -c -f - -C $(srcdir) macros | tar -x -f - -C $(distdir)
	find $(distdir)/macros -name CVS | xargs rm -rf

distclean-local:
	rm -f *~
	rm -rf autom4te.cache

# This line is added, because scrollkeeper leaves many files 
# in /var/scrollkeeper, that makes 'make distcheck' fail
distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper'

# Reformat the code.
indent:
	find . -name '*.[ch]' -exec indent -kr -i8 '{}' ';'

restorepo:
	cvs update -C po/*.po po/gnocatan.pot
