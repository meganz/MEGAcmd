#!/bin/bash

##
 # @file build/update_changelogs.sh
 # @brief Updates debian.changelog and megacmd.changes for the current version.
 #        Run from the build/ directory before a release.
 #
##
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$SCRIPT_DIR"
BASEPATH=$SCRIPT_DIR/../

# Get current version from CMakeLists.txt
megacmd_VERSION=$(grep -Po "MEGACMD_.*_VERSION [0-9]*" "$BASEPATH/CMakeLists.txt" | awk '{print $2}' | paste -sd '.')

echo "MEGAcmd version: $megacmd_VERSION"

# Read the last version for which changelogs were generated
version_file="version"

if [ -s "$version_file" ]; then
    last_version=$(cat "$version_file")
else
    last_version="none"
fi

if [ "$last_version" = "$megacmd_VERSION" ]; then
    echo "Changelogs already up to date for version $megacmd_VERSION (see build/version). Nothing to do."
    exit 0
fi

# Update RPM changelog (megacmd.changes)
changelog="megacmd/megacmd.changes"
changelogold="megacmd/megacmd.changes.old"
if [ -f "$changelog" ]; then
    mv "$changelog" "$changelogold"
fi
./generate_rpm_changelog_entry.sh "$megacmd_VERSION" "$BASEPATH/src/megacmdversion.h" > "$changelog"
if [ -f "$changelogold" ]; then
    cat "$changelogold" >> "$changelog"
    rm "$changelogold"
fi
echo "Updated $changelog"

# Update DEB changelog (debian.changelog)
changelog="megacmd/debian.changelog"
changelogold="megacmd/debian.changelog.old"
if [ -f "$changelog" ]; then
    mv "$changelog" "$changelogold"
fi
./generate_deb_changelog_entry.sh "$megacmd_VERSION" "$BASEPATH/src/megacmdversion.h" > "$changelog"
if [ -f "$changelogold" ]; then
    cat "$changelogold" >> "$changelog"
    rm "$changelogold"
fi
echo "Updated $changelog"

# Record the version so we don't regenerate on subsequent runs
echo "$megacmd_VERSION" > "$version_file"
echo "Done. Version recorded as $megacmd_VERSION in build/version."
