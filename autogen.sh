#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="Pioneers"

REQUIRED_AUTOCONF_VERSION="2.68"
REQUIRED_AUTOMAKE_VERSION="1.11"
REQUIRED_INTLTOOL_VERSION="0.35"

(test -f $srcdir/configure.ac \
  && test -f $srcdir/ChangeLog \
  && test -d $srcdir/client \
  && test -d $srcdir/server) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "gnome-common not found, using the included version"
    $srcdir/macros/gnome-autogen.sh "$@"
    exit 0
}

if test "X$1" = "X--fhs"; then
	shift
	gnome-autogen.sh --prefix=/usr --bindir=/usr/games --mandir=/usr/share/man "$@"
else
	gnome-autogen.sh "$@"
fi
