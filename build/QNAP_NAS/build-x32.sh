#!/usr/bin/env bash
export ARCH=x86
export BUILD_ARCH=32
export HOST=i686-QNAP-linux-gnu
export PATH=/opt/cross-project/CT/$HOST/cross-tools/bin:$PATH
export ConfigOpt="--host $HOST"
export PLATFORM=$ARCH

source build-common.sh

build_all

