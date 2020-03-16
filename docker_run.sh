#!/usr/bin/env bash

usage() {
    echo "Usage: $0 <video> <stats>"
    echo
    echo "<video> -- path to video file"
    echo "<stats> -- path to output stats file (.json or .json.bz2)"
    exit 1
}

if [ $# -ne 2 ]; then
    usage
fi

videoFile="$1"
statsFile="$2"

videoFileBasename="$(basename $1)"
statsFileBasename="$(basename $2)"

videoDir="$(realpath "$(dirname "$1")")"
statsDir="$(realpath "$(dirname "$2")")"

docker run \
    --rm \
    -v "$videoDir":/tmp/video \
    -v "$statsDir":/tmp/stats \
    -t videoparser \
    /tmp/video/"$videoFileBasename" \
    --output /tmp/stats/"$statsFileBasename"

echo "Stats written to: $statsFile"