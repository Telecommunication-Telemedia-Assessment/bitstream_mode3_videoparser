#!/usr/bin/env python3
# Bitream Mode 3 Video Parser
# Python Interface
#
# Authors:
#   * Werner Robitza,
#   * Steve Göring,
#   * Rakesh Rao Ramachandra Rao,
#   * Peter List

import argparse
import bz2
import gzip
import json
import os
import sys
import textwrap

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
        epilog="2017--2025",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("input", type=str, help="input video")
    parser.add_argument(
        "--dll",
        type=str,
        default="../VideoParser/libvideoparser.so",
        help="Path to DLL",
    )
    parser.add_argument(
        "--output",
        type=str,
        default=None,
        help=textwrap.dedent(
            """
            Path to output .json stats file.
            Use .ldjson for line-delimited JSON that is written as a frame is parsed.
            A file extension of .bz2 will compress it.
            If None report filename will be automatically estimated based on video name.
            """
        ),
    )
    parser.add_argument(
        "--num-frames",
        type=int,
        default=None,
        help="Number of frames to parse, default: all",
    )

    a = vars(parser.parse_args())

    video_parser = videoparser.VideoParser(a["input"], a["dll"], a["num_frames"])

    global output_file
    output_file = a["output"]
    if not output_file:
        output_file = os.path.splitext(os.path.basename(a["input"]))[0] + ".json.bz2"

    use_ldjson = ".ldjson" in output_file

    if use_ldjson:
        video_parser.set_frame_callback(frame_parsed)

    video_parser.parse()

    # write stats
    if not use_ldjson:
        print("Writing stats to output file: " + output_file)
        stats = video_parser.get_stats()
        with file_open(output_file, "w") as outfile:
            json.dump(stats, outfile, indent=4, sort_keys=True)


def frame_parsed(frame_info):
    with file_open(output_file, "a") as outfile:
        json.dump(frame_info, outfile)
        outfile.write("\n")


if __name__ == "__main__":
    main()
