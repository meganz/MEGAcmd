#!/bin/sh
##
# @file clean.sh
# @brief removes all temporary, generated files
#
# TODO: license
##
TARGETS="aclocal.m4 autom4te.cache config.guess config.log config.status config.sub configure depcomp install-sh libmega.pc libtool ltmain.sh Makefile Makefile.in missing stamp-h1 test-driver texput.log .deps clean compile
examples/.deps examples/.dirstamp examples/linux/.deps examples/linux/.dirstamp examples/linux/.libs examples/linux/megafuse examples/linux/*.o examples/*.o
examples/megacli examples/megasimplesync examples/.libs
include/Makefile include/Makefile.in
include/mega/config.h include/mega/config.h.in include/mega/stamp-h1
m4/libtool.m4 m4/lt~obsolete.m4  m4/ltoptions.m4  m4/ltsugar.m4  m4/ltversion.m4
src/*.lo
src/.libs
src/libmega.la
src/thread/.deps src/thread/.dirstamp src/thread/.libs src/thread/*.lo
src/.deps src/.dirstamp
src/crypto/.deps src/crypto/.dirstamp src/crypto/.libs src/crypto/*.lo
src/db/.deps src/db/.dirstamp src/db/.libs src/db/*.lo
src/gfx/.deps src/gfx/.dirstamp src/gfx/.libs src/gfx/*.lo
src/posix/.deps src/posix/.dirstamp src/posix/.libs src/posix/*.lo
src/win32/.deps src/win32/.dirstamp
tests/.deps tests/.dirstamp
doc/api doc/sphinx_api doc/_build
megacli1 megacli2 sync_in sync_out out
sdk_build
"
for file in $TARGETS
do
    rm -fr $file
done
