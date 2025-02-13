#!/bin/bash
set -e

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <platform>"
    exit 1
fi

PLATFORM="$1"
CONTAINER="megacmd-${PLATFORM}-build-env"

cleanup() {
    echo "Cleaning up container..."
    docker rm -f $CONTAINER > /dev/null 2>&1 || true
}
trap cleanup EXIT

echo "Starting build for '${PLATFORM}'..."

BUILDER_PARAMETER="${DOCKER_CUSTOM_BUILDER:+--builder $DOCKER_CUSTOM_BUILDER}"

docker buildx build $BUILDER_PARAMETER -t $CONTAINER \
  -f "$PWD/build/SynologyNAS/dockerfile/synology-cross-build.dockerfile" \
  "$PWD" \
  --build-arg PLATFORM="$PLATFORM"

CONTAINER_ID=$(docker create $CONTAINER)
docker cp "${CONTAINER_ID}:/image/${PLATFORM}" $PWD/build/SynologyNAS/packages/$PLATFORM

echo "Extracted package for '${PLATFORM}'"
