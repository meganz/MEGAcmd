##
 # @file Makefile.am
 # @brief an input file for automake tool
 # (c) 2013 by Mega Limited, Auckland, New Zealand
 #
 # This file is part of the MEGAcmd.
 #
 # MEGAcmd is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 #
 # @copyright Simplified (2-clause) BSD License.
 #
 # You should have received a copy of the license along with this
 # program.
##

AUTOMAKE_OPTIONS=foreign 1.9 subdir-objects
ACLOCAL_AMFLAGS=-I m4

AM_CPPFLAGS = \
	-I$(top_srcdir)/include -I$(top_srcdir)/third_party/utf8proc

#include m4/aminclude.am

AM_CPPFLAGS+=-I$(top_srcdir)/src -I$(top_srcdir)/sdk/include

if WIN32
AM_CPPFLAGS+=-I$(top_srcdir)/sdk/include/mega/win32
else
AM_CPPFLAGS+=-I$(top_srcdir)/sdk/include/mega/posix
endif

if ANDROID
AM_LIBTOOLFLAGS="--tag=CXX"
endif

if LINUX
AM_LIBTOOLFLAGS="--tag=CXX"
endif

if DRAGONFLY
AM_LIBTOOLFLAGS="--tag=CXX"
endif

if NETBSD
AM_LIBTOOLFLAGS="--tag=CXX"
endif

VERSION_INFO=$(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

CLEANFILES=
DISTCLEANFILES=
BUILT_SOURCES=
noinst_PROGRAMS=

SUBDIRS = sdk
include src/include.am

#EXTRA_DIST += autogen.sh
#EXTRA_DIST += libmega.pc.in
#EXTRA_DIST += $(DX_CONFIG)
#EXTRA_DIST += m4/ax_prog_doxygen.m4
#EXTRA_DIST += m4/aminclude.am

#pkgconfigdir=$(libdir)/pkgconfig
#pkgconfig_DATA=libmega.pc

#DISTCLEANFILES+=*~ libmega.pc
