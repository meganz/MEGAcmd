#!/usr/bin/env bash

build_all()
{
    # Purge prior build artifacts.
    clean || die "Couldn't remove prior build artifacts"

    # Make sure AR, CC and friends are correctly set.
    fix_environment

    # Perform the build.
    build_sdk || die "Couldn't build the SDK"
    build_cmd || die "Couldn't build MEGAcmd"

    # Package the build.
    package || die "Unable to create package"
}

build_cmd()
{
    local prefix="$PWD/sdk/sdk_build/install"
    local flag_gtest="--with-gtest=$prefix"
    local flag_freeimage="--with-freeimage=$prefix"
    local flag_tests="--enable-tests"
    local result=0

    ./autogen.sh

    result=$?

    if [ $result -ne 0 ]; then
        return $result
    fi

    use_freeimage || flag_freeimage="--without-freeimage"

    if ! use_gtest; then
        flag_gtest="--without-gtest"
        flag_tests="--disable-tests"
    fi

    env AR=$AR \
        ARCH=$ARCH \
        CC=$CC \
        CFLAGS="$(compiler_flags $CFLAGS)" \
        CPPFLAGS="$(preprocessor_flags $CFLAGS $CPPFLAGS)" \
        CXX=$CXX \
        CXXFLAGS="$(compiler_flags $CXXFLAGS)" \
        LD=$LD \
        NM=$NM \
        OBJCXX=$CXX \
        OBJDUMP=$OBJDUMP \
        RANLIB=$RANLIB \
        STRIP=$STRIP \
      ./configure $ConfigOpt \
                  --disable-curl-checks \
                  --disable-examples \
                  --disable-shared \
                  --enable-inotify \
                  --enable-static \
                  --enable-sync \
                  --with-cares="$prefix" \
                  --with-cryptopp="$prefix" \
                  --with-curl="$prefix" \
                  --with-libmediainfo="$prefix" \
                  --with-libuv="$prefix" \
                  --with-libzen="$prefix" \
                  --with-openssl="$prefix" \
                  --with-readline="$prefix" \
                  --with-sodium="$prefix" \
                  --with-sqlite="$prefix" \
                  --with-termcap="$prefix" \
                  --with-zlib="$prefix" \
                  --without-ffmpeg \
                  --without-libraw \
                  --without-pcre \
                  --without-pdfium \
                  $flag_gtest \
                  $flag_freeimage \
                  $flag_tests

    result=$?

    if [ $result -ne 0 ]; then
        return $result
    fi

    make

    result=$?

    return $result
}

build_sdk()
{
    local flag_configure_only="-c"
    local flag_cross_compiling="-X"
    local flag_disable_examples="-n"
    local flag_disable_freeimage=""
    local flag_enable_cares="-e"
    local flag_enable_cryptopp="-q"
    local flag_enable_curl="-g"
    local flag_enable_megaapi="-a"
    local flag_enable_readline="-R"
    local flag_enable_sodium="-u"
    local flag_enable_tests="-T"
    local flag_enable_uv="-v"
    local flag_openssl_assembly=""
    local flag_openssl_compiler="linux-generic$BUILD_ARCH"
    local result=0

    pushd sdk || return

    mkdir -p sdk_build/build || return
    mkdir -p sdk_build/install/lib || return

    use_assembly  || flag_openssl_assembly="no-asm"
    use_gtest     || flag_enable_tests=""
    use_freeimage || flag_disable_freeimage="-f"

    env AR=$AR \
        ARCH=$ARCH \
        CC=$CC \
        CFLAGS="$(compiler_flags $CFLAGS)" \
        CPPFLAGS="$(preprocessor_flags $CFLAGS $CPPFLAGS)" \
        CXX=$CXX \
        CXXFLAGS="$(compiler_flags $CXXFLAGS)" \
        LD=$LD \
        NM=$NM \
        OBJCXX=$CXX \
        OBJDUMP=$OBJDUMP \
        RANLIB=$RANLIB \
        STRIP=$STRIP \
      ./contrib/build_sdk.sh -C "$ConfigOpt" \
                             -O "$flag_openssl_compiler" \
                             -S "$flag_openssl_assembly" \
                             $flag_configure_only \
                             $flag_cross_compiling \
                             $flag_disable_examples \
                             $flag_disable_freeimage \
                             $flag_enable_cares \
                             $flag_enable_cryptopp \
                             $flag_enable_curl \
                             $flag_enable_megaapi \
                             $flag_enable_readline \
                             $flag_enable_sodium \
                             $flag_enable_tests \
                             $flag_enable_uv

    local result=$?

    popd

    return $result
}

clean()
{
    if [ -f Makefile ]; then
        make distclean || return
    fi

    rm -rf $(package_directory)
}

die()
{
    echo "ERROR: $1"
    exit 1
}

compiler_flags()
{
    local result="$*"

    # Strip preprocessor flags.
    result="$(echo $result | sed -e 's/ *-[DI][^ ]\+//g')"

    # Strip NEON flags (as it breaks freeimage.)
    result="$(echo $result | sed -e 's/-mfpu=neon[^ ]*//g')"

    # GCC's ARM ABI changed between 7.0 and 7.1.
    result="$result -Wno-psabi"

    echo "$result"
}

fix_environment()
{
    export CC=$HOST-gcc
    export CXX=$HOST-g++

    for tool in ar ld nm ranlib strip; do
        export ${tool^^}=$HOST-$tool
    done
}

package()
{
    directory=$(package_directory)

    mkdir -p $directory || return

    cp mega-cmd \
       mega-cmd-server \
       mega-exec \
       src/client/mega-* \
       $directory \
       || return

    sed -i -e "s/^QPKG_VER=.*/QPKG_VER=\"$(version)\"/" \
        $(package_root)/qpkg.cfg
}

package_directory()
{
    echo "$(package_root)/$PLATFORM"
}

package_root()
{
    echo build/QNAP_NAS/megacmdpkg
}

preprocessor_flags()
{
    echo "$*" | grep -o -E -- '-[DI][^ ]+' | xargs
}

use_assembly()
{
    test "$ARCH" != "x86_64"
}

use_gtest()
{
    false
}

use_freeimage()
{
    true
}

version()
{
    major=$(version_component MAJOR)
    minor=$(version_component MINOR)
    micro=$(version_component MICRO)

    echo "$major.$minor.$micro"
}

version_component()
{
    sed -ne "s/.*$1_VERSION \([0-9]\+\).*/\1/p" \
        src/megacmdversion.h
}

# Make sure we're at the root of the MEGAcmd source tree.
cd ../..

