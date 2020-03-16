#!/bin/bash

build_ffmpeg() {
    echo "Configure, Build and Install ffmpeg"
    cd ffmpeg
    ./configure_ffmpeg.sh
    make -j "$(nproc)"
    make
    make install
    cd ..
}

build_videoparser() {
    echo "Build videoparser Application and libvideoparser"
    cd VideoParser
    scons
    cd ..
}

parse_test_videos() {
    if [[ ! -d "test_videos" ]]; then
        exit 0
    fi
    echo "Parse some test videos"
    for i in  test_videos/*.mkv; do
        echo "Parse $i"
        ./parser.sh "$i" --output "$(basename "$i" .mkv).json.bz2"
    done
}

cd "$(dirname "$(readlink -f "$0" || realpath "$0")")"
build_ffmpeg
build_videoparser
#parse_test_videos
