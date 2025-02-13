#!/bin/bash
set -e
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )


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

BUILDER_PARAMETER="${DOCKER_CUSTOM_BUILDER:+--builder $DOCKER_CUSTOM_BUILDER --load}"
MEGACMD_FOLDER="${SOURCE_PATH:-${SCRIPT_DIR}/../..}"
OUTPUT_FOLDER="${OUTPUT_PATH:-$PWD/build/SynologyNAS/packages}"

docker buildx build $BUILDER_PARAMETER -t $CONTAINER \
  -f "$SCRIPT_DIR/dockerfile/synology-cross-build.dockerfile" \
  "${MEGACMD_FOLDER}" \
  --build-arg PLATFORM="$PLATFORM"

CONTAINER_ID=$(docker create $CONTAINER)
mkdir -p "$OUTPUT_FOLDER"
docker cp "${CONTAINER_ID}:/image/${PLATFORM}" "${OUTPUT_FOLDER}"/

echo "Extracted package for '${PLATFORM}'"
