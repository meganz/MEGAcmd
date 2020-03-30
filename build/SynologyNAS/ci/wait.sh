#!/bin/bash
# This script waits for the build to finish by checking for the modified 
# time stamps of `start` and `result`
set -ex
thisdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

function modified_time_s {
  date +%s -r $1 2>/dev/null || echo -1
}

while [ `modified_time_s $thisdir/start` -gt `modified_time_s $thisdir/result` ]; do
  sleep 1
done
echo "Build finished!"
