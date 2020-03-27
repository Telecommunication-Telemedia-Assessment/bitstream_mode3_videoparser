# Bitstream Mode3 Videoparser

The following repository consists of a video parser that can extract video statistics without decoding pixel information from a given video file. Here, statistics refer to higher mode statistics, considering e.g. motion, block sizes, QP values, ...

For a direct application of this videoparser, please check out the [ITU-T P.1204.3 Reference Implementation](https://github.com/Telecommunication-Telemedia-Assessment/bitstream_mode3_p1204_3).

If you use this videoparser in any of your research work, please cite the following paper:

```
@inproceedings{rao2020p1204,
  author={Rakesh Rao {Ramachandra Rao} and Steve G\"oring and Werner Robitza and Alexander Raake and Bernhard Feiten and Peter List and Ulf Wüstenhagen},
  title={Bitstream-based Model Standard for 4K/UHD: ITU-T P.1204.3 -- Model Details, Evaluation, Analysis and Open Source Implementation},
  BOOKTITLE={2020 Twelfth International Conference on Quality of Multimedia Experience (QoMEX)},
  address="Athlone, Ireland",
  days=26,
  month=May,
  year=2020,
}
```

## Requirements

To build the videoparser on a Linux system (e.g. Ubuntu 18.04 or newer) you need the following main requirements:

* Python 3
* `scons` build system
* `gcc`

All requirements can be installed with the following commands:

```bash
sudo apt-get update -qq
sudo apt-get -y -qq install python3 python3-numpy python3-pip git scons autoconf automake build-essential libass-dev libfreetype6-dev libsdl2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev pkg-config texinfo wget zlib1g-dev yasm
pip3 install --user --upgrade pip
pip3 install --user pandas
```

If you want to run the parser under Windows, please check out [the Development guide](./development.md).

## Building

To finally build the parser, either run:

```bash
./build_and_test.sh
```

Or, manually run:

```bash
# Configure, Build and Install ffmpeg
cd ffmpeg && ./configure_ffmpeg.sh && make -j $(nproc) && make install

# Build videoparser Application and libvideoparser
cd ../VideoParser && scons
```

## Usage

To run the parser, call:

```bash
./parser.sh <video>
```

For help, see `./parser.sh --help`:

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

## Troubleshooting

If something is not working, please run:

```bash
./testmain.sh <video>
```

It will open a GDB run of the main video parser library. Type `run` and check if something breaks.


## Development

To further add new codecs or measures, please see [development.md](./development.md).


## Authors

Main developers:

* Peter List – Deutsche Telekom AG
* Anton Schubert – Technische Universität Ilmenau
* Steve Göring – Technische Universität Ilmenau
* Rakesh Rao Ramachandra Rao – Technische Universität Ilmenau
* Werner Robitza – Technische Universität Ilmenau

Contributors:

* Alexander Raake – Technische Universität Ilmenau
* Bernhard Feiten – Deutsche Telekom AG
* Ulf Wüstenhagen – Deutsche Telekom AG


## License

This video parser is based on several marked changes in FFmpeg and additional developed software.
The FFmpeg software is licensed under the GNU Lesser General Public License version 2.1 (LGPL v2.1+), see `ffmpeg/COPYING.LGPLv2.1`.
In addition, all non-FFmpeg related sotware parts are also licensed under LGPL v2.1+, see `LICENSE.md`.

