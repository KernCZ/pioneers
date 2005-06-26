#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="Pioneers"

# Tested with automake1.4 and automake1.7
# The next two lines are needed because gnome-common only tries to use
# automake-1.4, even when a newer version is present.
REQUIRED_AUTOMAKE_VERSION="any"
automake_progs="automake-1.8 automake-1.7 automake-1.4"
 

(test -f $srcdir/configure.ac \
  && test -f $srcdir/ChangeLog \
  && test -d $srcdir/client \
  && test -d $srcdir/server) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

# Create acinclude.m4 from the extra macro
cp macros/type_socklen_t.m4 acinclude.m4

which gnome-autogen.sh || {
#    echo "You need to install gnome-common from the GNOME CVS"
    echo "gnome-common not found, reverting to old macros directory"
    . $srcdir/macros/autogen.sh
    exit 1
}
USE_GNOME2_MACROS=1 . gnome-autogen.sh
