AC_INIT([hif], [1.0.0], [ctor@hif-cli.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC

AC_SUBST([AM_CFLAGS], ["-g -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wunreachable-code -Wno-unused-function -Wno-cpp -Wall -Werror -pedantic"])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])

AC_SEARCH_LIBS([sqlite3_open], [sqlite3], [], [
  AC_MSG_ERROR([unable to find the sqlite3_open() function])
])
AC_OUTPUT

