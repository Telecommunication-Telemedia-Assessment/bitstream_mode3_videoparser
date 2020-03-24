# Bitstream Mode3 Videoparser
The following repository consists of a video parser that can extract video statistics without decoding pixel information from a given vidoe file, here the statistics refer to higher mode statistics, considering e.g. motion, block sizes, qp values, ....

For a direct application of this videoparser please checkout [ITU-T P.1204.3 Reference Implementation](https://github.com/Telecommunication-Telemedia-Assessment/bitstream_mode3_p1204_3).


## Requirements
To build the videoparser on a linux system (e.g. Ubuntu 18.04 or newer) you need the following main requirements:

* python 3
* scons build system
* gcc

All requirements can be installed with the following commands:
```bash
sudo apt-get update -qq
sudo apt-get install -y -qq python3 python3-numpy python3-pip git scons
sudo apt-get -y install autoconf automake build-essential libass-dev libfreetype6-dev libsdl2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev pkg-config texinfo wget zlib1g-dev yasm
sudo pip3 install --upgrade pip
sudo pip3 install pandas
```


If you want to run the parser under Windows please checkout [development.md](./development.md).

## Building
To finally build the parser run `build_and_test.sh`, or

```bash
# Configure, Build and Install ffmpeg
cd ffmpeg && ./configure_ffmpeg.sh && make -j $(nproc) && make install

# Build videoparser Application and libvideoparser
cd ../VideoParser && scons
```

Now you can call `./parser.sh <video>` or `./parser.sh --help`

```
usage: parse.py [-h] [--dll DLL] [--output OUTPUT] input

Bitstream Mode 3 Video Parser

positional arguments:
  input            input video

optional arguments:
  -h, --help       show this help message and exit
  --dll DLL        Path to DLL (default: ../VideoParser/libvideoparser.so)
  --output OUTPUT  Path to output JSON stats file, a file extension of
                   .json.bz2 will compress it; if None report filename will be
                   autoamtically estimated based on video name (default: None)

2017--2020
```

If something is not working please run:
```
./testmain.sh <video>
```
It will open a GDB run of the main video parser library, to type `run` and check if something breaks.


## Development
To further add new codecs or measures, please see [development.md](./development.md).


## Authors

* Peter List (Deutsche Telekom),
* Anton Schubert (TU Ilmenau),
* Steve Göring (TU Ilmenau),
* Rakesh Rao Ramachandra Rao (TU Ilmenau),
* Werner Robitza (TU Ilmenau)


## License
This video parser is based on several marked changes in FFmpeg and additional developed software.
The FFmpeg software is under the GNU Lesser General Public License version 2.1 (LGPL v2.1+), see `ffmpeg/COPYING.LGPLv2.1`.
In addition all non FFmpeg related sotware parts are also under LGPL v2.1+, see `LICENCE.md`.

