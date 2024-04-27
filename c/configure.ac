AC_PREREQ([2.71])
AC_INIT([arping],[0.01],[thomas@habets.se])
AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR([src/zipbrute.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_LIB([m], [log10])

AC_DEFUN([CHECK_COMPILER_OPTION], [
    AC_MSG_CHECKING([compiler option $1])
    OLD_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $1"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
    [AC_MSG_RESULT(yes)],
    [
        AC_MSG_RESULT(no)]
        CFLAGS="$OLD_CFLAGS"
    )
])

CHECK_COMPILER_OPTION([-D_GNU_SOURCE])
CHECK_COMPILER_OPTION([-O6])
CHECK_COMPILER_OPTION([-fomit-frame-pointer])
CHECK_COMPILER_OPTION([-march=native])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $debug
  C Compiler.....: $CC $CFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
"
