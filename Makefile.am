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

if BUILD_CLIENT
CLIENT_SUBDIRS = client
endif

#if BUILD_SERVER
SERVER_SUBDIRS = server
#endif

if BUILD_META_SERVER
META_SERVER_SUBDIRS = meta-server
endif

if BUILD_EDITOR
EDITOR_SUBDIRS = editor
endif

SUBDIRS = po common $(CLIENT_SUBDIRS) $(SERVER_SUBDIRS) $(META_SERVER_SUBDIRS) $(EDITOR_SUBDIRS) docs macros
DIST_SUBDIRS = po common client server meta-server editor docs macros

EXTRA_DIST = autogen.sh pioneers.spec xmldocs.make omf.make README.Cygwin README.MinGW

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
