# Pioneers - Implementation of the excellent Settlers of Catan board game.
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
AC_INIT([pioneers],[0.9.36],[pio-develop@lists.sourceforge.net])
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_SRCDIR([client])
AM_CONFIG_HEADER(config.h)
 
PROTOCOL_VERSION=0.9
META_PROTOCOL_VERSION=1.2
PIONEERS_DEFAULT_GAME_PORT=5556
PIONEERS_DEFAULT_GAME_HOST=localhost
PIONEERS_DEFAULT_ADMIN_PORT=5555
PIONEERS_DEFAULT_META_PORT=5557
PIONEERS_DEFAULT_META_SERVER=pioneers.debian.net

## If GLIB_REQUIRED_VERSION is set to 2.6,
## check the code for HAVE_GLIB_2_6
GLIB_REQUIRED_VERSION=2.4
GTK_REQUIRED_VERSION=2.4
 
AM_MAINTAINER_MODE

AC_PROG_LIBTOOL

AC_PROG_CC
AC_HEADER_STDC

if test $USE_MAINTAINER_MODE = yes; then
	pioneers_warnings=yes;
	pioneers_debug=yes;
	pioneers_logging=yes;
	pioneers_deprecationChecks=yes;
else
	pioneers_warnings=no;
	pioneers_debug=no;
	pioneers_logging=no;
	pioneers_deprecationChecks=no;
fi

AC_ARG_ENABLE([warnings],
[  --enable-warnings       Compile with check for compiler warnings (gcc-only).],
[case "${enableval}" in
  full) pioneers_warnings=full;;
  yes)  pioneers_warnings=yes ;;
  "")   pioneers_warnings=yes ;;
  *)    pioneers_warnings=no  ;;
esac])

AC_ARG_ENABLE([debug],
[  --enable-debug          Enable debug information.],
[case "${enableval}" in
  yes)  pioneers_debug=yes ;;
  "")   pioneers_debug=yes ;;
  *)    pioneers_debug=no  ;;
esac])

AC_ARG_ENABLE([logging],
[  --enable-logging        Enable network logging.],
[case "${enableval}" in
  yes)  pioneers_logging=yes ;;
  "")   pioneers_logging=yes ;;
  *)    pioneers_logging=no  ;;
esac])

AC_ARG_ENABLE([deprecation-checks],
[  --enable-deprecation-checks
                          Enable strict deprecation checks.],
[case "${enableval}" in
  yes)  pioneers_deprecationChecks=yes ;;
  "")   pioneers_deprecationChecks=yes ;;
  *)    pioneers_deprecationChecks=no  ;;
esac])

## The warnings are in the same order as in 'man gcc'
if test "x$GCC" = xyes; then
	if test "$pioneers_warnings" = yes -o "$pioneers_warnings" = full; then
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wall")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -W")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wpointer-arith")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wcast-qual")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wwrite-strings")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wno-sign-compare") # Turn off
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Waggregate-return")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wstrict-prototypes")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wmissing-prototypes")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wmissing-declarations")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wredundant-decls")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -Wnested-externs")
		AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -O")
	fi
	if test "$pioneers_warnings" = full; then
		flags="-Wfloat-equal";
		flags="$flags -Wdeclaration-after-statement";
		flags="$flags -Wundef";
		flags="$flags -Wendif-labels";
		flags="$flags -Wshadow";
		flags="$flags -Wbad-function-cast";
		flags="$flags -Wconversion";
		flags="$flags -Wold-style-definition";
		flags="$flags -Wunreachable-code";
		flags="$flags -pedantic";

		# This for loop comes from gnome-compiler-flags.m4
		for option in $flags; do
			SAVE_CFLAGS="$CFLAGS"
			CFLAGS="$CFLAGS $option"
			AC_MSG_CHECKING([whether gcc understands $option])
			AC_TRY_COMPILE([], [],
				has_option=yes,
				has_option=no,)
			CFLAGS="$SAVE_CFLAGS"
			AC_MSG_RESULT($has_option)
			if test $has_option = yes; then
				AC_SUBST(AM_CFLAGS, "$AM_CFLAGS $option")
			fi
			unset has_option
			unset SAVE_CFLAGS
		done
		unset option
		unset flags
	fi
fi

if test "$pioneers_debug" = yes; then
	AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -ggdb3")
fi

if test "$pioneers_logging" = yes; then
	AC_DEFINE_UNQUOTED(DEBUG, [1], [Defined when logging is enabled])
fi

if test "$pioneers_deprecationChecks" = yes; then
	## TODO: These defines should be in AM_CPPFLAGS instead of AM_CFLAGS
	AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DG_DISABLE_DEPRECATED")
	AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DGDK_DISABLE_DEPRECATED")
	AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DGTK_DISABLE_DEPRECATED")
	AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DGNOME_DISABLE_DEPRECATED")
fi
	
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
if test $have_scrollkeeper = yes; then
	AC_DEFINE(HAVE_HELP, 1,
		[Defined when online help is present])
fi

# glib is always needed
PKG_CHECK_MODULES(GLIB2, glib-2.0 >= $GLIB_REQUIRED_VERSION)

PKG_CHECK_MODULES(GLIB2_6, glib-2.0 >= 2.6, 
	have_glib_2_6=yes, have_glib_2_6=no)
if test x$have_glib_2_6 = xyes; then
	AC_DEFINE(HAVE_GLIB_2_6, [1], [Defined if GLib 2.6 or newer exists])
fi

# Gtk+ support
PKG_CHECK_MODULES(GTK2, gtk+-2.0 >= $GTK_REQUIRED_VERSION, have_gtk2=yes, 
	[PKG_CHECK_MODULES(OLD_GTK2, gtk+-2.0,
		[have_gtk2="no, Gtk+ version too old"],
		[have_gtk2="no, Gtk+ not installed"])
	])
AM_CONDITIONAL(HAVE_GTK2, [test "$have_gtk2" = "yes"])

# libgnome is only needed for help and old settingsfile
if test "$have_gtk2" = "yes"; then
	if test "$have_scrollkeeper" = "yes"; then
		need_libgnome=yes;
	else
		if test "$have_glib_2_6" = "yes"; then
			need_libgnome=no;
		else
			need_libgnome=yes;
		fi
	fi
else
	need_libgnome=no;
fi

if test "$need_libgnome" = "yes"; then
	# libgnome-2.0 support
	PKG_CHECK_MODULES(GNOME2, libgnome-2.0, 
		[have_gnome="yes"], 
		[have_gnome="no, libgnome-2.0 not installed"])
	if test $have_gnome = yes; then
		AC_DEFINE(HAVE_LIBGNOME, 1, 
			[Defined is libgnome is present and needed])
	fi
else
	have_gnome=$have_gtk2;
	AC_SUBST(GNOME2_CFLAGS)
	AC_SUBST(GNOME2_LIBS)
fi
AM_CONDITIONAL(HAVE_GNOME, [test "$have_gnome" = "yes"])

AC_ARG_ENABLE([admin-gtk],
[  --enable-admin-gtk      Turn on (unstable) network administration support.],
[case "${enableval}" in
  yes) admin_gtk_support=yes  ;;
  "")  admin_gtk_support=yes  ;;
  *)   admin_gtk_support=no ;;
esac], [admin_gtk_support=no])
AM_CONDITIONAL(ADMIN_GTK_SUPPORT, [test x$admin_gtk_support = xyes])

AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([netdb.h fcntl.h netinet/in.h sys/socket.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([syslog.h],
	[pioneers_have_syslog=yes;],
	[pioneers_have_syslog=no;])
AC_CHECK_HEADERS([sys/param.h])
AC_HEADER_SYS_WAIT
AC_HEADER_TIME

AC_C_CONST

# Functions
AC_FUNC_FORK
AC_FUNC_SELECT_ARGTYPES

# Mathematics
AC_CHECK_FUNCS([rint sqrt])
# String functions
AC_CHECK_FUNCS([strrchr strspn strstr strcspn])
AC_CHECK_FUNCS([memmove memset])
# Network and I/O functions
AC_CHECK_FUNCS([gethostname select socket])
getsockopt_arg3="void *";
# getaddrinfo and friends
AC_CHECK_FUNCS([getaddrinfo gai_strerror freeaddrinfo], 
	AC_DEFINE_UNQUOTED(HAVE_GETADDRINFO_ET_AL, [1],
		[Defined when getaddrinfo. gai_strerror and freeaddrinfo are present]))

# The Windows ports (Cygwin and MinGW) are client-only
pioneers_is_mingw_port=no;
case $host in
	*-*-cygwin*)
		pioneers_is_windows_port=yes;;
	*-*-mingw*)
		pioneers_is_windows_port=yes;
		pioneers_is_mingw_port=yes;;
	*)
		pioneers_is_windows_port=no;;
esac

# Can a non-blocking socket be created?
pioneers_have_non_blocking_sockets=no;
AC_CHECK_FUNCS([fcntl],[pioneers_have_non_blocking_sockets=yes])
if test "$pioneers_have_non_blocking_sockets" = "no"; then
	# Only check for ws2tcpip now,
	# because it will cause problems under Cygwin
	AC_CHECK_HEADERS([ws2tcpip.h],
		[pioneers_have_non_blocking_sockets=yes;
		 getsockopt_arg3="char *";
		])
fi
AC_DEFINE_UNQUOTED(GETSOCKOPT_ARG3, $getsockopt_arg3, [The type of the third argument of getsockopt])

if test $pioneers_is_mingw_port = yes; then
	# The check for WSACleanup in ws2_32 needs an include of ws2tcpip.h
	# This is not possible with the AC_CHECK_LIB macro
	# AC_CHECK_LIB(ws2_32, WSACleanup)
	# Just add ws2_32 to the list of libraries
	AC_SUBST(LIBS, "-lws2_32 $LIBS")
	AC_SUBST(AM_CFLAGS, "-mms-bitfields $AM_CFLAGS")
	AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DWIN32_LEAN_AND_MEAN")
	if test $pioneers_logging = no; then
		# No console window for the graphical applications
		AC_SUBST(GTK2_LIBS, "$GTK2_LIBS -mwindows")
	fi
	# Don't use bin, lib and share subdirectories
	datadir='${prefix}'
	bindir='${prefix}'
	libdir='${prefix}'
	pioneers_datadir=.
	pioneers_themedir_install=$datadir/themes
	pioneers_themedir=themes
	pioneers_localedir=locale
else
	pioneers_datadir=$datadir
	pioneers_themedir_install=$datadir/games/pioneers/themes
	pioneers_themedir=$pioneers_themedir_install
	pioneers_localedir=$libdir/locale
fi
AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DDATADIR=\\\""$pioneers_datadir"\\\"")
AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DTHEMEDIR=\\\""$pioneers_themedir"\\\"")
AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DLOCALEDIR=\\\""$pioneers_localedir"\\\"")
AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DPIONEERS_DIR_DEFAULT=\\\""$pioneers_datadir/games/pioneers"\\\"")
AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DPIONEERS_SERVER_CONSOLE_PATH=\\\""$bindir/pioneers-server-console"\\\"")
AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DPIONEERS_SERVER_GTK_PATH=\\\""$bindir/pioneers-server-gtk"\\\"")
AC_SUBST(AM_CFLAGS, "$AM_CFLAGS -DPIONEERS_AI_PATH=\\\""$bindir/pioneersai"\\\"")
AC_SUBST(pioneers_themedir, $pioneers_themedir_install)

if test $pioneers_is_windows_port = yes; then
	AC_SUBST(WINDOWS_ICON, [windowsicon.o])
fi

# Time out
AC_CHECK_FUNCS([alarm])
# Functions needed for the hack to override the language
AC_CHECK_FUNCS([setlocale])

# Data types
AC_STRUCT_TM
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
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
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_GAME_PORT, "$PIONEERS_DEFAULT_GAME_PORT",
	[The default port for a new game])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_GAME_HOST, "$PIONEERS_DEFAULT_GAME_HOST",
	[The default host for a new game])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_ADMIN_PORT, "$PIONEERS_DEFAULT_ADMIN_PORT",
	[The default port for the admin interface])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_META_PORT, "$PIONEERS_DEFAULT_META_PORT",
	[The port for the meta server])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_META_SERVER, 
	"$PIONEERS_DEFAULT_META_SERVER", [The default meta server])

## internationalization support
ALL_LINGUAS="de es fr hu nl it"

GETTEXT_PACKAGE=pioneers
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext package name])
AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED(ALL_LINGUAS, "$ALL_LINGUAS", [Available languages])

# All checks are completed.
# Determine which executables cannot be built
pioneers_build_client_ai=yes;
pioneers_build_client_gtk=yes;
pioneers_build_editor=yes;
pioneers_build_server_console=yes;
pioneers_build_server_gtk=yes;
pioneers_build_metaserver=yes;

if test $pioneers_have_syslog = no; then
	pioneers_build_metaserver=no;
fi
if test $have_gnome != yes; then
	pioneers_build_client_gtk=no;
	pioneers_build_editor=no;
	pioneers_build_server_gtk=no;
fi
if test $pioneers_have_non_blocking_sockets = no; then
	pioneers_build_client_ai=no;
	pioneers_build_client_gtk=no;
	pioneers_build_server_console=no;
	pioneers_build_server_gtk=no;
	pioneers_build_metaserver=no;
fi

# The server functionality is not ported to MS Windows
if test $pioneers_is_windows_port = yes; then
	pioneers_build_server_console=no;
	pioneers_build_server_gtk=no;
	pioneers_build_metaserver=no;
fi

AM_CONDITIONAL(BUILD_CLIENT, [test $pioneers_build_client_gtk = yes -o $pioneers_build_client_ai = yes])
AM_CONDITIONAL(BUILD_EDITOR, [test $pioneers_build_editor = yes])
AM_CONDITIONAL(BUILD_SERVER, [test $pioneers_build_server_gtk = yes -o $pioneers_build_server_console = yes])
AM_CONDITIONAL(BUILD_META_SERVER, [test $pioneers_build_metaserver = yes])

AC_CONFIG_FILES([ \
	Makefile \
	pioneers.spec \
	po/Makefile.in \
	common/Makefile \
	client/Makefile \
	client/common/Makefile \
	client/ai/Makefile \
	server/Makefile \
	meta-server/Makefile \
	editor/Makefile \
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
	server/gtk/Makefile \
	editor/gtk/Makefile
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
        Build graphical client             $pioneers_build_client_gtk
        Build computer player              $pioneers_build_client_ai
        Build game editor                  $pioneers_build_editor
        Build graphical server             $pioneers_build_server_gtk
        Build console server               $pioneers_build_server_console
        Build metaserver                   $pioneers_build_metaserver
        Build help                         $have_scrollkeeper
    Developers only:
        Use compiler warnings              $pioneers_warnings
        Add debug information              $pioneers_debug
        Enable network logging             $pioneers_logging
        Enable deprecation checks          $pioneers_deprecationChecks
])
