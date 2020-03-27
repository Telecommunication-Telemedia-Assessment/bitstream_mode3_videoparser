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


cd "$(dirname "$(readlink -f "$0" || realpath "$0")")"
build_ffmpeg
build_videoparser
