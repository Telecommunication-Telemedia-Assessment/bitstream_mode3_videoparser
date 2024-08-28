FROM --platform=linux/amd64 ubuntu:22.04

ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -qq update && apt-get install -qq -y \
  python3 python3-pip git \
  python3-venv \
  scons ffmpeg \
  autoconf automake \
  build-essential libass-dev \
  libfreetype6-dev libsdl2-dev \
  libtheora-dev libtool \
  libva-dev libvdpau-dev \
  libvorbis-dev libxcb1-dev \
  libxcb-shm0-dev libxcb-xfixes0-dev \
  pkg-config texinfo \
  wget zlib1g-dev yasm && \
  rm -rf /var/lib/apt/lists/*

COPY . /

RUN pip3 install --user pandas

# patch for 22.04
RUN cd ffmpeg && patch -p1 < mathops.patch

# build
RUN ./build.sh

ENTRYPOINT ["/parser.sh"]
