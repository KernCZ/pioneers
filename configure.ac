# Gnocatan - Implementation of the excellent Settlers of Catan board game.
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

AC_INIT
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR([client])

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([gnocatan], [0.8.1.22])

PROTOCOL_VERSION="\"0.8.0\""
META_PROTOCOL_VERSION="\"1.0\""

AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_HEADER_STDC([])
AM_PROG_LIBTOOL

AC_TRY_LINK_FUNC(g_rand_new_with_seed,
	[AC_DEFINE(HAVE_G_RAND_NEW_WITH_SEED, [],
               [Define if there is a g_rand() function in glib.])],
	[MT_RAND_OBJ="mt_rand.o"]
)

AC_ARG_ENABLE([admin-gtk],
[  --enable-admin-gtk      Turn on (unstable) network administration support.],
[case "${enableval}" in
  yes) admin_gtk_support=true  ;;
  "")  admin_gtk_support=true  ;;
  *)   admin_gtk_support=false ;;
esac], [admin_gtk_support=false])
AM_CONDITIONAL(ADMIN_GTK_SUPPORT, [test x$admin_gtk_support = xtrue])

AC_ARG_ENABLE([debug],
[  --enable-debug          Compile with lots of warnings and have verbose logging.],
[case "${enableval}" in
  yes) debug_support=true  ;;
  "")  debug_support=true  ;;
  *)   debug_support=false ;;
esac], [debug_support=false])
AM_CONDITIONAL(DEBUG, [test x$debug_support = xtrue])

AC_CHECK_HEADERS([getopt.h])
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(MT_RAND_OBJ)
AC_DEFINE_UNQUOTED(PROTOCOL_VERSION, $PROTOCOL_VERSION,
                   [Protocol version used by the program])
AC_DEFINE_UNQUOTED(META_PROTOCOL_VERSION, $META_PROTOCOL_VERSION,
                   [Protocol version used by the meta server])

## internationalization support
ALL_LINGUAS="de es fr nl"
AM_GNU_GETTEXT(use-libtool)
AC_LINK_FILES($nls_cv_header_libgt, $nls_cv_header_intl)
AC_DEFINE_UNQUOTED(ALL_LINGUAS, "$ALL_LINGUAS", [Available languages])

# gnome-2.0 support
PKG_CHECK_MODULES(GNOME2, libgnomeui-2.0)
AC_SUBST(GNOME2_CFLAGS)
AC_SUBST(GNOME2_LIBS)

PKG_CHECK_MODULES(GLIB2, glib-2.0)
AC_SUBST(GLIB2_CFLAGS)
AC_SUBST(GLIB2_LIBS)

AC_CONFIG_FILES([
        Makefile
        po/Makefile.in
        intl/Makefile
        common/Makefile
        client/Makefile
        client/common/Makefile
	client/gtk/Makefile
	client/gtk/data/Makefile
	client/gtk/data/themes/Makefile
	client/gtk/data/themes/FreeCIV-like/Makefile
	client/gtk/data/themes/Iceland/Makefile
	client/gtk/data/themes/Tiny/Makefile
	client/ai/Makefile
        client/help/Makefile
        client/help/C/Makefile
        server/Makefile
        meta-server/Makefile
        ai/Makefile
        docs/Makefile
        stamp.h])
dnl AC_CONFIG_COMMANDS([default],[sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile])
AC_OUTPUT
