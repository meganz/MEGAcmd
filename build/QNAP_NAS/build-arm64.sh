#!/usr/bin/env bash
export ARCH=aarch64
export BUILD_ARCH=64
export HOST=aarch64-QNAP-linux-gnu
export PATH=/opt/cross-project/$HOST/bin:$PATH
export ConfigOpt="--host $HOST"
export PLATFORM=arm_64

source build-common.sh

build_all

