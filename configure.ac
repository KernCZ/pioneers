# Gnocatan - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 the Free Software Foundation
# Copyright (C) 2003 Bas Wijnen <b.wijnen@phys.rug.nl>
# Copyright (C) 2004-2005 Roland Clobus <rclobus@bigfoot.com>
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

AC_PREREQ(2.53)
AC_INIT([gnocatan],[0.9.3])
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_SRCDIR([client])
AM_CONFIG_HEADER(config.h)
 
PROTOCOL_VERSION=0.9
META_PROTOCOL_VERSION=1.1
GNOCATAN_DEFAULT_GAME_PORT=5556
GNOCATAN_DEFAULT_GAME_HOST=localhost
GNOCATAN_DEFAULT_ADMIN_PORT=5555
GNOCATAN_DEFAULT_META_PORT=5557
GNOCATAN_DEFAULT_META_SERVER=gnocatan.debian.net
 
AM_MAINTAINER_MODE

AC_PROG_LIBTOOL

AC_PROG_CC
AC_HEADER_STDC

## Scrollkeeper dependency test taken from gnome-games 2.6.2
## Begin tests for scrollkeeper
# SCROLLKEEPER_REQUIRED is never used?
SCROLLKEEPER_REQUIRED=0.3.8
AC_SUBST(SCROLLKEEPER_REQUIRED)

AC_PATH_PROG(SCROLLKEEPER_CONFIG, scrollkeeper-config,no)
if test x$SCROLLKEEPER_CONFIG = xno; then 
	have_scrollkeeper=no;
else
	have_scrollkeeper=yes;
fi
AM_CONDITIONAL(HAVE_SCROLLKEEPER, [test x$have_scrollkeeper = xyes])

# gnome-2.0 support
PKG_CHECK_MODULES(GNOME2, libgnomeui-2.0, have_gnome=yes, have_gnome=no)
AC_SUBST(GNOME2_CFLAGS)
AC_SUBST(GNOME2_LIBS)
AM_CONDITIONAL(HAVE_GNOME, [test x$have_gnome = xyes])

# glib is always needed
PKG_CHECK_MODULES(GLIB2, glib-2.0)
AC_SUBST(CFLAGS, "$CFLAGS $GLIB2_CFLAGS")
AC_SUBST(LIBS, "$LIBS $GLIB2_LIBS")

AC_TRY_COMPILE([#include <glib.h>],
	[g_rand_new_with_seed(0)],
	have_glib_random=yes,
	have_glib_random=no)

AC_ARG_ENABLE([old-random],
[  --enable-old-random     Use the old random generator.],
[case "${enableval}" in
  yes)  use_old_random=yes ;;
  "")   use_old_random=yes ;;
  *)    use_old_random=no ;;
esac], [use_old_random=no])
if test x$have_glib_random = xno; then
  use_old_random=yes
fi
if test x$use_old_random = xyes; then
  MT_RAND_OBJ="mt_rand.o"
else
  AC_DEFINE(HAVE_G_RAND_NEW_WITH_SEED, [],
		[Define if there is a g_rand() function in glib.])
fi

AC_ARG_ENABLE([admin-gtk],
[  --enable-admin-gtk      Turn on (unstable) network administration support.],
[case "${enableval}" in
  yes) admin_gtk_support=yes  ;;
  "")  admin_gtk_support=yes  ;;
  *)   admin_gtk_support=no ;;
esac], [admin_gtk_support=no])
AM_CONDITIONAL(ADMIN_GTK_SUPPORT, [test x$admin_gtk_support = xyes])

AC_ARG_ENABLE([debug],
[  --enable-debug          Compile with lots of warnings and have verbose logging.],
[case "${enableval}" in
  yes) debug_support=yes ;;
  "")  debug_support=yes ;;
  *)   debug_support=no  ;;
esac], [debug_support=no])
AM_CONDITIONAL(DEBUG, [test x$debug_support = xyes])

AC_CHECK_HEADERS([getopt.h])
# Check if socklen_t is present
TYPE_SOCKLEN_T

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(MT_RAND_OBJ)
# Defines, accessible for all source files
AC_DEFINE_UNQUOTED(PROTOCOL_VERSION, "$PROTOCOL_VERSION",
                   [Protocol version used by the program])
AC_DEFINE_UNQUOTED(META_PROTOCOL_VERSION, "$META_PROTOCOL_VERSION",
                   [Protocol version used by the meta server])
AC_DEFINE_UNQUOTED(GNOCATAN_DEFAULT_GAME_PORT, "$GNOCATAN_DEFAULT_GAME_PORT",
	[The default port for a new game])
AC_DEFINE_UNQUOTED(GNOCATAN_DEFAULT_GAME_HOST, "$GNOCATAN_DEFAULT_GAME_HOST",
	[The default host for a new game])
AC_DEFINE_UNQUOTED(GNOCATAN_DEFAULT_ADMIN_PORT, "$GNOCATAN_DEFAULT_ADMIN_PORT",
	[The default port for the admin interface])
AC_DEFINE_UNQUOTED(GNOCATAN_DEFAULT_META_PORT, "$GNOCATAN_DEFAULT_META_PORT",
	[The port for the meta server])
AC_DEFINE_UNQUOTED(GNOCATAN_DEFAULT_META_SERVER, 
	"$GNOCATAN_DEFAULT_META_SERVER", [The default meta server])

## internationalization support
ALL_LINGUAS="de es fr nl it"

GETTEXT_PACKAGE=gnocatan
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext package name])
AM_GLIB_GNU_GETTEXT
ac_sources="$nls_cv_header_libgt"
ac_dests="$nls_cv_header_intl"
while test -n "$ac_sources"; do
  set $ac_dests; ac_dest=$1; shift; ac_dests=$*
  set $ac_sources; ac_source=$1; shift; ac_sources=$*
  ac_config_links_1="$ac_config_links_1 $ac_dest:$ac_source"
done
AC_CONFIG_LINKS([$ac_config_links_1])
AC_DEFINE_UNQUOTED(ALL_LINGUAS, "$ALL_LINGUAS", [Available languages])

AC_CONFIG_FILES([ \
	Makefile \
	gnocatan.spec \
	po/Makefile.in \
	common/Makefile \
	client/Makefile \
	client/common/Makefile \
	client/ai/Makefile \
	server/Makefile \
	meta-server/Makefile \
	docs/Makefile \
	])
AC_CONFIG_FILES([ \
	common/gtk/Makefile \
	client/gtk/Makefile \
	client/gtk/data/Makefile \
	client/gtk/data/themes/Makefile \
	client/gtk/data/themes/FreeCIV-like/Makefile \
	client/gtk/data/themes/Iceland/Makefile \
	client/gtk/data/themes/Tiny/Makefile \
	server/gtk/Makefile
	])
AC_CONFIG_FILES([ \
	client/help/Makefile \
	client/help/C/Makefile \
	])

AC_OUTPUT

AC_MSG_NOTICE([
$PACKAGE v$VERSION configuration:

        Source code location:              ${srcdir}
        Compiler:                          ${CC}
        Build GNOME/GTK+ applications      $have_gnome
        Build help                         $have_scrollkeeper
    Compatibility:
        Use old random generator           $use_old_random
    Developers only:
        Use verbose DEBUG logging          $debug_support
])
