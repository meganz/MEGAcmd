#!/bin/bash
set -e

#If a dependency required an older commit (older than baseline), set it here:
VCPKG_OLDEST_COMMIT=c6d3ab273572019ea0d6f6688e1163143190d326

if [ -z ${VCPKG_OLDEST_COMMIT+x} ]; then
    commit=$(grep '."builtin-baseline"' vcpkg.json  | awk -F '"' '{print $(NF-1)}')
else
    commit="${VCPKG_OLDEST_COMMIT}"
fi
commit_date=$(curl -s "https://api.github.com/repos/microsoft/vcpkg/commits/$commit" | grep '"date"' |  head -n 1 | awk -F '"' '{print $(NF-1)}')
git clone --shallow-since $commit_date https://github.com/microsoft/vcpkg $@
