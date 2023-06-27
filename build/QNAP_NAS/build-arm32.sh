#!/usr/bin/env bash
export ARCH=arm
export BUILD_ARCH=32
export HOST=arm-linux-gnueabihf
export PATH=/opt/cross-project/arm/linaro/bin:$PATH
export ConfigOpt="--host $HOST"
export PLATFORM=arm-x41

source build-common.sh

build_all

