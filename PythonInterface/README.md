# VideoParser Python Interface

Author: Werner Robitza, Steve Göring

Requirements:

* Python 3
* Pip packages: numpy, pandas

Usage:

    parse.py [-h] [--dll DLL] [--output OUTPUT] input

    positional arguments:
      input            input video

    optional arguments:
      -h, --help       show this help message and exit
      --dll DLL        Path to DLL (default: ../VideoParser/libvideoparser.so)
      --output OUTPUT  Path to output JSON stats file (default: stats.json.bz2)

Test suite:

    python parse.py ../TestMain/test-h264.mp4 --output stats-h264.json
    python parse.py ../TestMain/test-h265.mp4 --output stats-h265.json
    python parse.py ../TestMain/test-vp9.webm --output stats-vp9.json

## For Developers

Use:

    clang2py VideoStat.h

to get a starting point for the C header definitions.