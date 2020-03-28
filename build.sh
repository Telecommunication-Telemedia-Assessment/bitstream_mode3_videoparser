#!/bin/bash

build_ffmpeg() {
    echo "configure, build and install ffmpeg"
    cd ffmpeg
    ./configure_ffmpeg.sh
    make -j "$(nproc)"
    make
    make install
    cd ..
}

build_videoparser() {
    echo "build videoparser application and libvideoparser"
    cd VideoParser
    scons
    cd ..
}

check_scons() {
    if ! command -v scons 2>/dev/null; then
        echo "'scons' not found. Please install 'scons' first!"
        exit 1
    fi
}

cd "$(dirname "$(readlink -f "$0" || realpath "$0")")"
check_scons
build_ffmpeg
build_videoparser
