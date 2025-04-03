# syntax=docker/dockerfile:1
FROM debian:12-slim as base

RUN rm -f /etc/apt/apt.conf.d/docker-clean; \
    echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' > /etc/apt/apt.conf.d/keep-cache

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update \
    && apt-get install -y --no-install-recommends \
    cmake zip pkg-config curl python3 python3-pip autoconf-archive nasm git g++ uuid-runtime zstd fuse \
    && python3 -m pip install unittest-xml-reporting pexpect --break-system-packages

RUN if [ ! -e /etc/machine-id ]; then \
        uuidgen > /etc/machine-id; \
    fi

FROM base as build-deps-cmake

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update \
    && apt-get install -y --no-install-recommends \
    ccache jq \
    cmake zip pkg-config curl python3 autoconf-archive nasm libgtest-dev libgmock-dev git g++ make unzip autoconf  ca-certificates automake ninja-build \
    libfuse-dev libtool

COPY vcpkg.json ./vcpkg.json
COPY build/clone_vcpkg_from_baseline.sh ./clone_vcpkg_from_baseline.sh
RUN /clone_vcpkg_from_baseline.sh /vcpkg

FROM scratch as src

WORKDIR /usr/src/megacmd
COPY sdk ./sdk
COPY src ./src
COPY build ./build
COPY contrib ./contrib
COPY tests/common ./tests/common
COPY tests/integration ./tests/integration
COPY tests/unit ./tests/unit
COPY CMakeLists.txt ./CMakeLists.txt
COPY vcpkg.json ./vcpkg.json

FROM build-deps-cmake as build
LABEL stage=autocleanable-intermediate-stage
WORKDIR /usr/src/megacmd

# setting ccache as env variables breaks latest vcpkg libsodium compilation
#ENV CC "ccache gcc-12"
#ENV CXX "ccache g++-12"
ENV CCACHE_DIR /tmp/ccache
ENV VCPKG_DEFAULT_BINARY_CACHE /tmp/vcpkgcache
ARG ENABLE_asan=OFF
ARG ENABLE_ubsan=OFF
ARG ENABLE_tsan=OFF
ARG ENABLE_MEGACMD_TESTS=ON

COPY --from=src /usr/src/megacmd /usr/src/megacmd


#We don't wont a potential config coming from host machine to meddle with the build:
RUN rm ./sdk/include/mega/config.h || true

RUN --mount=type=cache,target=/tmp/ccache \
    --mount=type=cache,target=/tmp/vcpkgcache \
    --mount=type=tmpfs,target=/tmp/build \
     cmake -B /tmp/build \
    -DVCPKG_ROOT=/vcpkg \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DENABLE_ASAN=${ENABLE_asan} \
    -DENABLE_UBSAN=${ENABLE_ubsan} \
    -DENABLE_TSAN=${ENABLE_tsan} \
    -DENABLE_MEGACMD_TESTS=${ENABLE_MEGACMD_TESTS} \
    -DWITH_FUSE=ON \
    && cmake --build /tmp/build -j$(nproc) --target mega-cmd mega-cmd-server mega-exec \
    mega-cmd-updater mega-cmd-tests-integration mega-cmd-tests-unit \
    && cmake --install /tmp/build #|| mkdir /inspectme && mv /tmp/build/* /vcpkg  /inspectme

FROM base as final

COPY --from=build /usr/bin/mega* /usr/bin/
COPY --from=build /opt/megacmd /opt/megacmd
COPY --chmod=555 tests/*.py /usr/local/bin/
#COPY --from=build /inspectme /inspectme
