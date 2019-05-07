#!/bin/bash
# This script triggers the build by touching `start`
set -ex
thisdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
touch $thisdir/start
