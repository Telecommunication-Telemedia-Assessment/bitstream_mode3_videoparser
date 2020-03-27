#!/bin/bash

echo "run some tests"

parse_test_videos() {
    if [[ ! -d "test_videos" ]]; then
        echo "please create a folder ./test_videos where some h264, h265 or vp9 encoded videos are stored"
        exit 0
    fi
    echo "parse some test videos"
    for i in  test_videos/*.mkv; do
        echo "Parse $i"
        ./parser.sh "$i" --output "$(basename "$i" .mkv).json.bz2"
    done
}

parse_test_videos