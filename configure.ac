##
# @file configure.ac
# @brief an input file for autoconf tool
#
# (c) 2013- by Mega Limited, Auckland, New Zealand
#
# TODO: license
##

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.


m4_define([megacmd_major_version], [$(cat src/megacmdversion.h | grep "define MEGACMD_MAJOR" | cut -d" " -f 3)])
m4_define([megacmd_minor_version], [$(cat src/megacmdversion.h | grep "define MEGACMD_MINOR" | cut -d" " -f 3)])
m4_define([megacmd_micro_version], [$(cat src/megacmdversion.h | grep "define MEGACMD_MICRO" | cut -d" " -f 3)])

m4_define([megacmd_version],
          [megacmd_major_version.megacmd_minor_version.megacmd_micro_version])


# libtool interface versioning
m4_define([megacmd_lt_revision], [0])
m4_define([megacmd_lt_current], [$(grep define <src/megacmdversion.h | awk '{print @S|@3}' | awk 'BEGIN { RS = "" ; FS = "\n" } {printf @S|@1*10000+@S|@2*100+@S|@3}')])
m4_define([megacmd_lt_age], [0])

AC_INIT([MEGAcmd], 0.9.4)
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_CONFIG_FILES(Makefile)
AC_OUTPUT

AC_CONFIG_SUBDIRS(sdk)


# Define _GNU_SOURCE
# AC_GNU_SOURCE
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.11 foreign silent-rules])
#AC_CONFIG_HEADERS([megacmdconfig.h])
#~ AC_CONFIG_HEADERS([include/mega/config.h])
LT_INIT([shared disable-static win32-dll])
#~ AC_CONFIG_MACRO_DIR([m4])

# enable silent build
m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])

LT_CURRENT=megacmd_lt_current
LT_REVISION=megacmd_lt_revision
LT_AGE=megacmd_lt_age

AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_PID_T
AC_TYPE_OFF_T

AH_VERBATIM([__STDC_FORMAT_MACROS],
            [/* C99 says: define this to get the PRI... macros from stdint.h */
#ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS 1
#endif])

AC_CHECK_TYPES([ssize_t])

AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(OS, "${host}", [cpu-machine-OS])

# Check programs
AC_PROG_OBJCXX
AC_PROG_CXX
if test "$CXX" = no || test "$CXX:$GXX" = "g++:"; then
  AC_MSG_ERROR([C++ compiler not found !])
fi

AC_PROG_LIBTOOL
AM_SANITY_CHECK


# Check for cppcheck
AC_CHECK_PROG(HAVE_CPPCHECK, cppcheck, yes)
AM_CONDITIONAL(CPPCHECK, test -n "$HAVE_CPPCHECK")

# set C++
AC_LANG_CPLUSPLUS

# Check headers
AC_STDC_HEADERS
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_DIRENT

AC_DEFINE(__STDC_CONSTANT_MACROS, [], [Force definition of constant macros for C++])
AC_DEFINE(__STDC_FORMAT_MACROS, [], [Force definition of format macros for C++])
AC_DEFINE(__STDC_LIMIT_MACROS, [], [Force definition of limit macros for C++])

# Add 64-bits file support on some hosts
AC_SYS_LARGEFILE
# use new interfaces
AC_FUNC_FSEEKO
AC_TYPE_OFF_T

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_SIZEOF([uint64_t])
AC_STRUCT_TM
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h sys/socket.h sys/timeb.h htonl glob.h malloc.h sys/malloc.h malloc/malloc.h])

# Debug
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug], [enable support for running in debug mode]),
    [], [enable_debug=no])

# Enable debug flags / build
if test "x$enable_debug" = "xyes" ; then
    AM_CXXFLAGS="-Wall -g -ggdb3 -O0"
    CXXFLAGS="-Wall -g -ggdb3 -O0"
    AC_SUBST([AM_CXXFLAGS])
    AC_DEFINE(DEBUG, 1, [Define to enable debug logging])
else
    CXXFLAGS="$CXXFLAGS -DNDEBUG=1"
fi

#~ # Check if we can use -fPIC flag
#~ AX_CHECK_COMPILE_FLAG([-fPIC], [
  #~ AX_CHECK_LINK_FLAG([-fPIC],
    #~ [CXXFLAGS="$CXXFLAGS -fPIC"]
  #~ )
#~ ])

AM_CONDITIONAL(ENABLE_STATIC, test x$enable_static = xyes)
AC_SUBST(ENABLE_STATIC)

#termcap
AC_MSG_CHECKING(for termcap)
AC_ARG_WITH(termcap,
  AS_HELP_STRING(--with-termcap=PATH, base of termcap installation),
  [AC_MSG_RESULT($with_termcap)
   case $with_termcap in
   no)
     ;;
   yes)
    AC_CHECK_LIB([termcap], [tputs], [TERMCAP_LIBS="-ltermcap"],
        [AC_MSG_NOTICE([termcap library not found or not usable.])]
    )
    ;;
   *)

    # determine if library is installed
    if test -d "$with_termcap/lib"; then
        LDFLAGS="-L$with_termcap/lib $LDFLAGS"
        CXXFLAGS="-I$with_termcap/include $CXXFLAGS"
        CPPFLAGS="-I$with_termcap/include $CPPFLAGS"

        AC_CHECK_HEADERS([termcap.h],[
            TERMCAP_LDFLAGS="-L$with_termcap/lib"
            TERMCAP_CXXFLAGS="-I$with_termcap/include"
            TERMCAP_CPPFLAGS="-I$with_termcap/include"],
            AC_MSG_NOTICE([termcap.h header not found or not usable])
        )
    else
        LDFLAGS="-L$with_termcap $LDFLAGS"
        CXXFLAGS="-I$with_termcap $CXXFLAGS"
        CPPFLAGS="-I$with_termcap $CPPFLAGS"

        AC_CHECK_HEADERS([termcap.h],[
            TERMCAP_LDFLAGS="-L$with_termcap"
            TERMCAP_CXXFLAGS="-I$with_termcap"
            TERMCAP_CPPFLAGS="-I$with_termcap"],
            AC_MSG_NOTICE([termcap.h header not found or not usable])
        )
    fi

    AC_CHECK_LIB([termcap], [tputs], [TERMCAP_LIBS="-ltermcap"], [
        AC_MSG_NOTICE([termcap library not found or not usable.])])

    #restore
    LDFLAGS=$SAVE_LDFLAGS
    CXXFLAGS=$SAVE_CXXFLAGS
    CPPFLAGS=$SAVE_CPPFLAGS

    ;;
   esac
  ],
  [AC_MSG_RESULT([--with-termcap not specified])
    AC_CHECK_LIB([termcap], [tputs], [TERMCAP_LIBS="-ltermcap"],
        [AC_MSG_NOTICE([termcap library not found or not usable.])]
    )
  ]
)
AC_SUBST(TERMCAP_LDFLAGS)
AC_SUBST(TERMCAP_LIBS)
AC_SUBST(TERMCAP_CXXFLAGS)
AC_SUBST(TERMCAP_CPPFLAGS)

# ReadLine
AC_MSG_CHECKING(for Readline)
AC_ARG_WITH(readline,
  AS_HELP_STRING(--with-readline=PATH, base of Readline installation),
  [AC_MSG_RESULT($with_readline)
   case $with_readline in
   no)
    AC_MSG_ERROR([readline library is required for the sample client.])
     ;;
   yes)
    AC_CHECK_HEADERS([readline/readline.h],, [
        AC_MSG_ERROR([readline/readline.h header not found or not usable])
    ])
    # readline requires termcap (or ncurses)
    SAVE_LIBS=$LIBS
    LIBS="$TERMCAP_LIBS $LIBS"
    AC_CHECK_LIB([readline], [rl_save_prompt], [RL_LIBS="-lreadline"], [
        AC_MSG_ERROR([readline library is required for the sample client.])])
    LIBS=$SAVE_LIBS
     ;;
   *)

    # determine if library is installed
    if test -d "$with_readline/lib"; then
        LDFLAGS="-L$with_readline/lib $LDFLAGS"
        CXXFLAGS="-I$with_readline/include $CXXFLAGS"
        CPPFLAGS="-I$with_readline/include $CPPFLAGS"
        AC_CHECK_HEADERS([readline/readline.h], [
         RL_LDFLAGS="-L$with_readline/lib "
         RL_CXXFLAGS="-I$with_readline/include "],
         AC_MSG_ERROR([readline/readline.h header not found or not usable])
        )
    else
        LDFLAGS="-L$with_readline $LDFLAGS"
        CXXFLAGS="-I$with_readline $CXXFLAGS"
        CPPFLAGS="-I$with_readline $CPPFLAGS"
        AC_CHECK_HEADERS([readline/readline.h], [
         RL_LDFLAGS="-L$with_readline"
         RL_CXXFLAGS="-I$with_readline"],
         AC_MSG_ERROR([readline/readline.h header not found or not usable])
        )
    fi

    # readline requires termcap (or ncurses)
    SAVE_LIBS=$LIBS
    LIBS="$TERMCAP_LIBS $LIBS"
    AC_CHECK_LIB([readline], [rl_save_prompt], [RL_LIBS="-lreadline"], [
        AC_MSG_ERROR([readline library is required for the sample client.])])
    LIBS=$SAVE_LIBS

    #restore
    LDFLAGS=$SAVE_LDFLAGS
    CXXFLAGS=$SAVE_CXXFLAGS
    CPPFLAGS=$SAVE_CPPFLAGS
    ;;
   esac
  ],
  [AC_MSG_RESULT([--with-readline not specified])
    AC_CHECK_HEADERS([readline/readline.h],, [
        AC_MSG_ERROR([readline/readline.h header not found or not usable])
    ])
    # readline requires termcap (or ncurses)
    SAVE_LIBS=$LIBS
    LIBS="$TERMCAP_LIBS $LIBS"
    AC_CHECK_LIB([readline], [rl_save_prompt], [RL_LIBS="-lreadline"], [
        AC_MSG_ERROR([readline library is required for the sample client.])])
    LIBS=$SAVE_LIBS
  ]
  )
AC_SUBST(RL_LDFLAGS)
AC_SUBST(RL_LIBS)
AC_SUBST(RL_CXXFLAGS)


#
# PCRE
#

#libpcre
AC_MSG_CHECKING(for libpcrecpp)
pcre=false
AC_ARG_WITH(pcre,
  AS_HELP_STRING(--with-pcre=PATH, base of libpcrecpp installation),
  [
   case $with_pcre in
   no)
    pcre=false
     ;;
   yes)
    AC_CHECK_HEADERS([pcrecpp.h],, [
        AC_MSG_ERROR([pcrecpp.h header not found or not usable])
    ])
    AC_CHECK_LIB(pcrecpp, [main], [PCRE_LIBS="-lpcrecpp"],[
            AC_MSG_ERROR([Could not find libpcrecpp])
    ])
    AC_CHECK_LIB(pcre, [pcre_compile], [PCRE_LIBS="$PCRE_LIBS -lpcre"],[
            AC_MSG_ERROR([Could not find libpcre])
    ])
    pcre=true
     ;;
   *)

    # determine if library is installed
    if test -d "$with_pcre/lib64"; then
        LDFLAGS="-L$with_pcre/lib64 $LDFLAGS"
        CXXFLAGS="-I$with_pcre/include $CXXFLAGS"
        CPPFLAGS="-I$with_pcre/include $CPPFLAGS"

        AC_CHECK_HEADERS(pcrecpp.h,
         PCRE_CXXFLAGS="-I$with_pcre/include"
         PCRE_CPPFLAGS="-I$with_pcre/include"
         PCRE_LDFLAGS="-L$with_pcre/lib64",
         AC_MSG_ERROR([pcrecpp.h header not found or not usable])
         )
    else
        LDFLAGS="-L$with_pcre/lib32 $LDFLAGS"
        CXXFLAGS="-I$with_pcre/include $CXXFLAGS"
        CPPFLAGS="-I$with_pcre/include $CPPFLAGS"

        AC_CHECK_HEADERS(pcrecpp.h,
         PCRE_CXXFLAGS="-I$with_pcre/include"
         PCRE_CPPFLAGS="-I$with_pcre/include"
         PCRE_LDFLAGS="-L$with_pcre/lib32",
         AC_MSG_ERROR([pcrecpp.h header not found or not usable])
         )
    fi

    AC_CHECK_LIB(pcrecpp, [main], [PCRE_LIBS="-lpcrecpp"],[
            AC_MSG_ERROR([Could not find libpcrecpp])
    ])
    AC_CHECK_LIB(pcre, [pcre_compile], [PCRE_LIBS="$PCRE_LIBS -lpcre"],[
            AC_MSG_ERROR([Could not find libpcre])
    ])

    pcre=true

    #restore
    LDFLAGS=$SAVE_LDFLAGS
    CXXFLAGS=$SAVE_CXXFLAGS
    CPPFLAGS=$SAVE_CPPFLAGS
    ;;
   esac
  ],
  [AC_MSG_RESULT([--with-pcre not specified])
      AC_CHECK_HEADERS([pcrecpp.h], [pcre=true], [])
    AC_CHECK_LIB(pcrecpp, [main], [PCRE_LIBS="-lpcrecpp"],[])
    AC_CHECK_LIB(pcre, [pcre_compile], [PCRE_LIBS="$PCRE_LIBS -lpcre"],[])
  ])
AC_SUBST(PCRE_CXXFLAGS)
AC_SUBST(PCRE_CPPFLAGS)
AC_SUBST(PCRE_LDFLAGS)
AC_SUBST(PCRE_LIBS)

if test "x$pcre" = "xtrue" ; then
    AC_DEFINE(USE_PCRE, [1], [Define to use libpcre])
fi
AM_CONDITIONAL([USE_PCRE], [test "x$pcre" = "xtrue"])


#PTHREAD

# check for Pthreads
AC_ARG_ENABLE(posix-threads,
    AS_HELP_STRING(--disable-posix-threads, disable POSIX threads support),
    [enable_posix_threads=no],
    [enable_posix_threads=yes])
if test x$enable_posix_threads != xno; then
    AC_CHECK_LIB(pthread, pthread_create, [HAVE_PTHREAD=yes], [HAVE_PTHREAD=no])
else
    HAVE_PTHREAD=no
fi
if test "$HAVE_PTHREAD" = "yes"; then
    AC_DEFINE(USE_PTHREAD, 1, [Defined if pthreads are available])
        if test "$HAVE_PTHREAD" = "yes"; then
        SAVE_LDFLAGS="-pthread $SAVE_LDFLAGS"
        LDFLAGS="-pthread $LDFLAGS"
    fi    
fi
AM_CONDITIONAL([HAVE_PTHREAD], [test "${HAVE_PTHREAD}" = "yes"])


# Output
#AC_CONFIG_FILES([
# Makefile
#])
AC_OUTPUT
#~ AC_OUTPUT([sdk/Makefile])

AC_MSG_NOTICE([Configured to build MEGAcmd:

  SDK version:      megacmd_version
  Host setup:       ${host}
  Install prefix:   $prefix
  Compiler:         $CXX
  CXXFLAGS:         $CXXFLAGS
  LDFLAGS:          $LDFLAGS
  gcc hardening:    $enable_gcc_hardening
  debug:            $enable_debug
  static:           $enable_static
  sync subsystem:   $enable_sync
  chat:             $enable_chat
  MEGA API          $enable_megaapi
  example apps:     $enable_examples
  megacmd app:      $enable_megacmd

  inotify:          $enable_inotify
  posix threads:    $enable_posix_threads

  Python bindings:  $enable_python
  Python3 bindings: $USE_PYTHON3
  PHP bindings:     $enable_php
  SWIG_FLAGS_PHP:   $SWIG_FLAGS_PHP
  
  Java bindings:    $enable_java

  OpenSSL:          $LIBSSL_FLAGS $LIBSSL_LDFLAGS $LIBSSL_LIBS
  Crypto++:         $CRYPTO_CXXFLAGS $CRYPTO_LDFLAGS $CRYPTO_LIBS
  Sodium:           $SODIUM_CXXFLAGS $SODIUM_LDFLAGS $SODIUM_LIBS
  Zlib:             $ZLIB_CXXFLAGS $ZLIB_LDFLAGS $ZLIB_LIBS
  SQLite3:          $DB_CXXFLAGS $DB_LDFLAGS $DB_LIBS
  c-ares:           $CARES_FLAGS $CARES_LDFLAGS $CARES_LIBS
  cURL:             $LIBCURL_FLAGS $LIBCURL_LIBS
  FreeImage:        $FI_CXXFLAGS $FI_LDFLAGS $FI_LIBS
  Readline:         $RL_CXXFLAGS $RL_LDFLAGS $RL_LIBS
  Termcap:          $TERMCAP_CXXFLAGS $TERMCAP_LDFLAGS $TERMCAP_LIBS
  PCRE:             $PCRE_CXXFLAGS $PCRE_LDFLAGS $PCRE_LIBS
  LIBUV:            $LIBUV_CXXFLAGS $LIBUV_LDFLAGS $LIBUV_LIBS
])
