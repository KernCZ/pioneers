AC_INIT(client)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(gnocatan, 0.6.1)

PROTOCOL_VERSION=\"0.6.0\"

AM_MAINTAINER_MODE

AM_ACLOCAL_INCLUDE(macros)

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_STDC_HEADERS
AC_ARG_PROGRAM
AM_PROG_LIBTOOL

GNOME_INIT
GNOME_COMPILE_WARNINGS
AM_PATH_GLIB

AC_TRY_LINK_FUNC(g_rand_new_with_seed,
	[AC_DEFINE(HAVE_G_RAND_NEW_WITH_SEED)],
	[MT_RAND_OBJ="mt_rand.o"]
)

AC_ARG_ENABLE(admin-gtk,
[  --enable-admin-gtk      Turn on (unstable) network administration support.],
[case "${enableval}" in
  yes) admin_gtk_support=true  ;;
  "")  admin_gtk_support=true  ;;
  *)   admin_gtk_support=false ;;
esac], [admin_gtk_support=false])
AM_CONDITIONAL(ADMIN_GTK_SUPPORT, test x$admin_gtk_support = xtrue)

AC_CHECK_HEADERS(getopt.h)
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(MT_RAND_OBJ)
AC_DEFINE_UNQUOTED(PROTOCOL_VERSION, $PROTOCOL_VERSION)

dnl GNOME_SUPPORT_CHECKS

dnl ## internationalization support
dnl ALL_LINGUAS=""
dnl AM_GNU_GETTEXT
dnl AC_LINK_FILES($nls_cv_header_libgt, $nls_cv_header_intl)

AC_OUTPUT(
Makefile
dnl ## internationalization support
dnl po/Makefile.in
dnl intl/Makefile
macros/Makefile
dnl support/Makefile
common/Makefile
client/Makefile
client/help/Makefile
client/help/C/Makefile
server/Makefile
meta-server/Makefile
stamp.h)

dnl ## internationalization support
dnl stamp.h],[sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile])
