#!/bin/bash
set -e

commit=$(jq -r '."builtin-baseline"' vcpkg.json)
commit_date=$(curl -s "https://api.github.com/repos/microsoft/vcpkg/commits/$commit" | jq -r '.commit.author.date')
git clone --shallow-since $commit_date https://github.com/microsoft/vcpkg $@
