AC_INIT
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR([client])
AC_PREFIX_DEFAULT([`gnome-config --prefix 2>/dev/null || echo /usr/local`])

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([gnocatan], [0.7.2])

PROTOCOL_VERSION=\"0.7.0\"
META_PROTOCOL_VERSION=\"1.0\"

AM_MAINTAINER_MODE

AM_ACLOCAL_INCLUDE(macros)

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_HEADER_STDC([])
AM_PROG_LIBTOOL

GNOME_INIT
GNOME_COMPILE_WARNINGS
AM_PATH_GLIB

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

AC_CHECK_HEADERS([getopt.h])
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(MT_RAND_OBJ)
AC_DEFINE_UNQUOTED(PROTOCOL_VERSION, $PROTOCOL_VERSION,
                   [Protocol version used by the program])
AC_DEFINE_UNQUOTED(META_PROTOCOL_VERSION, $META_PROTOCOL_VERSION,
                   [Protocol version used by the meta server])

dnl GNOME_SUPPORT_CHECKS

## internationalization support
ALL_LINGUAS="de"
AM_GNU_GETTEXT(use-libtool)
AC_LINK_FILES($nls_cv_header_libgt, $nls_cv_header_intl)
AC_DEFINE_UNQUOTED(ALL_LINGUAS, "$ALL_LINGUAS", [Available languages])

AC_CONFIG_FILES([Makefile po/Makefile.in intl/Makefile macros/Makefile common/Makefile client/Makefile client/help/Makefile client/help/C/Makefile server/Makefile meta-server/Makefile ai/Makefile stamp.h])
dnl AC_CONFIG_COMMANDS([default],[sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile])
AC_OUTPUT
