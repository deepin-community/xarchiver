#!/bin/sh

test "$BASH" && set -o pipefail

echo > AUTHORS
echo > NEWS
mkdir m4

echo -n "Creating the build system... "

intltoolize --copy --force --automake || exit
libtoolize --copy --force --quiet || exit
aclocal || exit
autoheader || exit
automake --copy --force-missing --add-missing --gnu 2>&1 | sed "/installing/d" || exit
autoconf || exit

echo "done."

# clean up in order to keep repository small
# (will be needed if 'make dist' is used though)
rm AUTHORS NEWS INSTALL aclocal.m4 compile intltool-extract.in intltool-merge.in intltool-update.in
rm -r autom4te.cache m4
