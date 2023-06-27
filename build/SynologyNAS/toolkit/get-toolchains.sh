#!/bin/bash
set -o pipefail

# Prints a message and terminates the script.
die()
{
    local message="$1"

    echo "${message}"

    exit 1
}

# Checks if a command is present.
present()
{
    local program="$1"

    if ! command -v "${program}" >/dev/null; then
        die "Couldn't find ${program}"
    fi
}

# Get a list of toolchain URIs.
toolchains()
{
    local root="https://archive.synology.com/download/ToolChain/toolkit"
    local version="$1"
    local uri="${root}/${version}"

    curl "${uri}" 2>/dev/null | pup 'a[href$=.txz] attr{href}'
}

# Display usage.
usage()
{
    die "get-toolchains.sh -d DESTINATION -v X.Y"
}

# Make sure aria2 is present.
present aria2c

# Make sure pup is present.
present pup

# Make sure xargs is present.
present xargs

destination=""
version=""

# Parse arguments.
while getopts ":d:v:" arguments; do
    case "${arguments}" in
    d)
        destination="${OPTARG}"
        ;;
    v)
        version="${OPTARG}"
        ;;
    ?)
        usage
        ;;
    esac
done

# Did the user specify where we should store the toolchains?
test -n "${destination}" || die "You must specify a destination."

# Did the user specify a DSM version?
test -n "${version}" || die "You must specify a DSM version."

# Try and retrieve a list of toolchain URIs.
uris="$(toolchains ${version})"

# Couldn't get a list of toolchains.
test -n "${uris}" || die "Couldn't get toolchains for DSM${version}"

echo "Trying to download toolchains..."

# Try and download the toolchains.
aria2c --continue=true \
       --dir=${destination} \
       --force-sequential=true \
       ${uris} \
  || die "Couldn't download toolchains."

echo "Toolchains downloaded."

