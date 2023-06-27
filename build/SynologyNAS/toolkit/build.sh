#!/usr/bin/env bash

# Where we are.
declare -r TOOLKIT_DIR=/toolkit

# Where builds are performed.
declare -r BUILD_DIR=${TOOLKIT_DIR}/build_env

# Where we can find necessary patches.
declare -r PATCH_DIR=$TOOLKIT_DIR/patches

# Where we can find Synology's package scripts.
declare -r PKGSCRIPTS_DIR=${TOOLKIT_DIR}/pkgscripts-ng

# What version of DSM are we targeting?
declare -r DSM_VERSION=7.2

# For convenience.
declare -r ENVDEPLOY="${PKGSCRIPTS_DIR}/EnvDeploy -v ${DSM_VERSION}"
declare -r PKGCREATE="${PKGSCRIPTS_DIR}/PkgCreate.py -v ${DSM_VERSION}"

# Full list of platforms.
all_platforms()
{
    ${ENVDEPLOY} -l |& cut -c 54-
}

# Performs a build for the specified platform.
build()
{
    local -r platform="$1"
    local -r env_dir="$(environment_path ${platform})"
    local -r src_dir="${TOOLKIT_DIR}/source/MEGAcmd"

    # Wipe prior SDK build state.
    rm -rf ${src_dir}/MEGAcmd/sdk/sdk_build

    # Prepare build environment.
    deploy_environment ${platform}

    # Prepare for SDK build.
    mkdir -p ${env_dir}/tmp/megasdkbuild

    # Avoid downloading SDK's dependencies.
    cp ${TOOLKIT_DIR}/sdk_dep_tarballs/* \
       ${env_dir}/tmp/megasdkbuild

    # Create a standard place where patches can be found.
    mkdir -p ${env_dir}/tmp/megapatches

    # Make sure no stale patches are present.
    rm -f ${env_dir}/tmp/megapatches/*

    # Copy patches into environment, if any.
    if [ -d "${PATCH_DIR}" ]; then
        find ${PATCH_DIR} \
             -type f \
             -name '*.patch' \
             -exec cp {} ${env_dir}/tmp/megapatches \;
    fi

    # Kick off the build.
    if ${PKGCREATE} -p ${platform} MEGAcmd; then
        local -r result_dir="${TOOLKIT_DIR}/results/${platform}"

        mkdir -p ${result_dir}

        cp -av ${env_dir}/image/* ${result_dir}

        clean_environment ${platform}
    fi
}

# Platforms we'll be building for.
build_platforms()
{
    local -r info_dir="${PKGSCRIPTS_DIR}/include"

    # Output all non-x64 platforms.
    for platform in $(all_platforms); do
        info_path="${info_dir}/platform.${platform}"

        if ! grep -q 'ARCH="x86_64"' ${info_path}; then
            echo ${platform}
        fi
    done

    # Output a representative x64 platform.
    echo kvmx64
}

# Discards the environment for the specified platform.
clean_environment()
{
    local -r platform="$1"
    local -r env_dir="$(environment_path ${platform})"

    if grep -q "${env_dir}/proc" /etc/mtab; then
        umount ${env_dir}/proc
    fi

    rm -rf ${env_dir}
}

# Deploys the environment for the specified platform.
deploy_environment()
{
    local -r platform="$1"

    clean_environment ${platform}

    ${ENVDEPLOY} -D \
                 -p $1 \
                 -t ${TOOLKIT_DIR}/toolkit_tarballs
}

# Checks that a directory is present.
check_directory()
{
    if [ ! -d $1 ]; then
        echo "ERROR: Directory $1 does not exist."
        exit 1
    fi
}

# Path to the specified platform's environment.
environment_path()
{
    echo "${BUILD_DIR}/ds.$1-${DSM_VERSION}"
}

if [ "$(id -u)" != "0" ]; then
    echo "You must be root to run this script."
    exit 1
fi

check_directory $TOOLKIT_DIR
check_directory ${PKGSCRIPTS_DIR}
check_directory $TOOLKIT_DIR/sdk_dep_tarballs
check_directory $TOOLKIT_DIR/source/MEGAcmd
check_directory $TOOLKIT_DIR/toolkit_tarballs

# Clear prior build results.
rm -rf ${TOOLKIT_DIR}/results
platforms=$(build_platforms)

# Try and build the packages.
for platform in ${platforms}; do
    build ${platform}
done

# Check whether all the packages were built.
for platform in ${platforms}; do
    echo -n "${platform}: "

    if [ -d "${TOOLKIT_DIR}/results/${platform}" ]; then
        echo "OK"
    else
        echo "FAIL"
    fi
done

