# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright © 2007-2018 ANSSI. All Rights Reserved.
AC_INIT([clip-mdadm],[1.1.4],[clipos@ssi.gouv.fr])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile scripts/Makefile man/Makefile init/Makefile])

if test -z "$PKG_CONFIG"; then
	PKG_CONFIG=pkg-config
fi
AC_SUBST(PKG_CONFIG)
if "$PKG_CONFIG" --version > /dev/null 2>&1 ; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([*** $PKG_CONFIG missing ***

    \'$PKG_CONFIG --version\' failed to run - make sure it is inside one of the
    directories in your PATH environment variable!

    pkg-config comes with the developement packages for GTK+-2.2, available at:

    with the developement packages for GTK+-2.2, available at:

    http://www.gtk.org])
fi

AC_DEFUN([CLIP_REQUIRE], [
AC_MSG_CHECKING(that $1 (version >= $2) is installed)
if "$PKG_CONFIG" --atleast-version=$2 $1 ; then
  LIBS="$LIBS `$PKG_CONFIG --libs $1`"
  CFLAGS="$CFLAGS `$PKG_CONFIG --cflags $1`"
  AC_MSG_RESULT(yes) 
else
  AC_MSG_RESULT(no)
  if "$PKG_CONFIG" --exists $1 ; then
      AC_MSG_ERROR(Current version is only `"$PKG_CONFIG" "$1" --modversion`)
  else
      AC_MSG_ERROR(Package is not installed)
  fi
fi
])

CLIP_REQUIRE(clip, 1.2.3)

AC_OUTPUT
