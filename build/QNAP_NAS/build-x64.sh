#!/usr/bin/env bash
export ARCH=x86_64
export BUILD_ARCH=64
export HOST=x86_64-QNAP-linux-gnu
export PATH=/opt/cross-project/CT/$HOST/cross-tools/bin:$PATH
export ConfigOpt="--host $HOST"
export PLATFORM=$ARCH

source build-common.sh

build_all

