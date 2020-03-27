#!/usr/bin/env python3
# Bitream Mode 3 Video Parser
# Python Interface
#
# Authors:
#   * Werner Robitza,
#   * Steve Göring,
#   * Rakesh Rao Ramachandra Rao,
#   * Peter List

import sys
import os
import argparse
import json
import bz2
import gzip

from sys import platform
if platform == "linux" or platform == "linux2":
    lib_suffix = ".so"
elif platform == "darwin":
    lib_suffix = ".dylib"
elif platform == "win32":
    lib_suffix = ".dll"

sys.path.append(os.path.dirname(os.path.realpath(__file__)))

import lib.videoparser as videoparser


def file_open(filename, mode="r"):
    """
    Opens a file, and if you add bz2 to filename a
    compressed file will be opened
    """
    if "bz2" in filename:
        return bz2.open(filename, mode + "t")
    if "gz" in filename:
        return gzip.open(filename, mode + "t")
    return open(filename, mode)


def main():
    # argument parsing
    parser = argparse.ArgumentParser(
        description="Bitstream Mode 3 Video Parser",
        epilog="2017--2020",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("input", type=str, help="input video")
    parser.add_argument(
        "--dll",
        type=str,
        default="../VideoParser/libvideoparser" + lib_suffix,
        help="Path to DLL/.so/.dylib",
    )
    parser.add_argument(
        "--output",
        type=str,
        default=None,
        help="Path to output JSON stats file, a file extension of .json.bz2 will compress it; if None report filename will be automatically estimated based on video name",
    )

    a = vars(parser.parse_args())

    video_parser = videoparser.VideoParser(a["input"], a["dll"])
    video_parser.set_frame_callback(frame_parsed)
    video_parser.parse()

    if not a["output"]:
        a["output"] = os.path.splitext(os.path.basename(a["input"]))[0] + ".json.bz2"

    # write stats
    print("Writing stats to output file: " + a["output"])
    stats = video_parser.get_stats()
    with file_open(a["output"], "w") as outfile:
        json.dump(stats, outfile, indent=4, sort_keys=True)


def frame_parsed(frame_info):
    pass


if __name__ == "__main__":
    main()
