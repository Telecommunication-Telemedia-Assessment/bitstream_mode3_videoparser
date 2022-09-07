#!/bin/sh

set -e

./configure  \
  --prefix="../Outputs" \
  --cc=x86_64-linux-gnu-gcc \
  --cxx=x86_64-linux-gnu-g++\
  --ar=x86_64-linux-gnu-ar \
  --extra-cflags="-Wno-error=implicit-function-declaration" \
  --disable-programs \
  --disable-doc \
  --disable-stripping \
  --enable-shared \
  --enable-pthreads \
  --enable-debug=2 \
  --disable-avdevice \
  --disable-avfilter \
  --disable-swscale \
  --disable-swresample \
  --disable-nvenc \
  --disable-vaapi \
  --enable-decoder=aac \
  --enable-parser=aac \
  --enable-demuxer=aac \
  --enable-decoder=mp3 \
  --enable-demuxer=mp3 \
  --enable-decoder=vorbis \
  --enable-parser=vorbis \
  --enable-decoder=opus \
  --enable-parser=opus \
  --enable-decoder=h264 \
  --enable-decoder=mpeg4 \
  --enable-parser=h264 \
  --enable-demuxer=h264 \
  --enable-demuxer=m4v \
  --enable-demuxer=mpegts \
  --enable-decoder=hevc \
  --enable-parser=hevc \
  --enable-demuxer=hevc \
  --enable-decoder=vp9 \
  --enable-parser=vp9 \
  --enable-demuxer=matroska \
  --enable-protocol=file

