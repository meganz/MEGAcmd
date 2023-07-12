#!/usr/bin/env bash
for architecture in arm x; do
    for width in 32 64; do
        platform=$architecture$width
        result=OK

        ./build-$platform.sh &> $platform.log || result=FAIL

        echo "$platform: $result"
    done
done

