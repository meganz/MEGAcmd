# Dockerfile for cross-compiling MEGAcmd for Synology in its different architectures. See README.md for more info.
FROM debian:12-slim

# Set default architecture
ARG PLATFORM=alpine
ENV PLATFORM=${PLATFORM}

RUN apt-get --quiet=2 update && DEBCONF_NOWARNINGS=yes apt-get --quiet=2 install \
    aria2 \
    autoconf \
    autoconf-archive \
    build-essential \
    ccache \
    cmake \
    curl \
    fakeroot \
    git \
    nasm \
    pkg-config \
    python3 \
    tar \
    unzip \
    wget \
    xz-utils \
    zip \
    1> /dev/null

ENV CCACHE_DIR=/tmp/ccache
ENV VCPKG_DEFAULT_BINARY_CACHE=/tmp/vcpkgcache

# We don't wont a potential config coming from host machine to meddle with the build
RUN rm sdk/include/mega/config.h || true

WORKDIR /mega
RUN chmod 777 /mega

# Clone and checkout known pkgscripts baseline
RUN git clone https://github.com/SynologyOpenSource/pkgscripts-ng.git pkgscripts \
    && git -C ./pkgscripts checkout e1d9f52

COPY sdk/dockerfile/dms-toolchain.sh /mega/
COPY sdk/dockerfile/dms-toolchains.conf /mega/
COPY build/SynologyNAS/toolkit/source/MEGAcmd/SynoBuildConf/install /mega/spk_install.sh

RUN chmod +x /mega/dms-toolchain.sh
RUN chmod +x /mega/spk_install.sh

COPY vcpkg.json ./vcpkg.json
COPY build/clone_vcpkg_from_baseline.sh ./clone_vcpkg_from_baseline.sh
RUN /mega/clone_vcpkg_from_baseline.sh /mega/vcpkg

WORKDIR /mega/megacmd
COPY sdk ./sdk
COPY src ./src
COPY build ./build
COPY contrib ./contrib
COPY tests/common ./tests/common
COPY CMakeLists.txt ./CMakeLists.txt
COPY vcpkg.json ./vcpkg.json

RUN /mega/dms-toolchain.sh ${PLATFORM}

# Remove the dynamic version of libatomic to avoid compiling with it, which causes
# execution to fail in the NAS because the shared library object cannot be found
RUN find /mega/toolchain/ \( -type f -o -type l \) -name libatomic.so* -delete

RUN --mount=type=cache,target=/tmp/ccache \
    --mount=type=cache,target=/tmp/vcpkgcache \
    --mount=type=tmpfs,target=/tmp/build \
    cmake -B buildDMS \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DENABLE_MEGACMD_TESTS=OFF \
        -DENABLE_ASAN=OFF \
        -DENABLE_UBSAN=OFF \
        -DENABLE_TSAN=OFF \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DWITH_FUSE=OFF \
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/mega/${PLATFORM}.toolchain.cmake \
        -DVCPKG_OVERLAY_TRIPLETS=/mega \
        -DVCPKG_ROOT=/mega/vcpkg \
        -DVCPKG_TARGET_TRIPLET=${PLATFORM} && \
    cmake --build buildDMS

RUN export PLATFORM=${PLATFORM} && /mega/spk_install.sh
