#TODO: license


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
