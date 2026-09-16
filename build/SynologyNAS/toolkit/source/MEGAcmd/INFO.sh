#!/bin/bash

source /mega/pkgscripts/include/pkg_util.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
MEGACMD_ROOT="${SCRIPT_DIR}/../../../../.."

# Single-source the version from CMakeLists.txt, so it cannot drift from the one
# the binaries report. The trailing component is the Synology package revision,
# which is independent of the MEGAcmd version and is bumped by hand.
megacmd_version=$(grep -Po "(set\(MEGACMD_MAJOR_VERSION \K\d+|set\(MEGACMD_MINOR_VERSION \K\d+|set\(MEGACMD_MICRO_VERSION \K\d+)" \
    "${MEGACMD_ROOT}/CMakeLists.txt" | paste -sd '.' -)

if [ -z "$megacmd_version" ]; then
    echo "INFO.sh: could not read the MEGAcmd version from ${MEGACMD_ROOT}/CMakeLists.txt" >&2
    exit 1
fi

package="megacmdpkg"
version="${megacmd_version}-0001"
displayname="MEGAcmd"
maintainer="Mega NZ"
arch="$PLATFORM"
description="MEGAcmd command line tool.  Access your MEGA.nz secure cloud storage account and upload/download files, use its commands in scripts, automatically synchronise folders between your MEGA.nz account and your Synology NAS.  Connect to your NAS via ssh or Putty to use the MEGAcmd commands.  Run mega-help from the command line for documentation, or see our User Guide online."
os_min_ver="7.0-40000"
maintainer="Mega Ltd."
maintaner_url="https://MEGA.nz"
support_url="https://mega.nz/help"
helpurl="https://github.com/meganz/MEGAcmd/blob/master/UserGuide.md"
thirdparty="yes"
ctl_stop="no"
[ "$(caller)" != "0 NULL" ] && return 0
pkg_dump_info

