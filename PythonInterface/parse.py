#!/usr/bin/env python3
# T-Labs Video Parser
# Python Interface
#
# Author: Werner Robitza


import sys
import os
import argparse
import json
import bz2
import gzip

sys.path.append(os.path.dirname(os.path.realpath(__file__)))

import lib.videoparser as videoparser


def file_open(filename, mode="r"):
    """ Open a file, and if you add bz2 to filename a compressed file will be opened
    """
    if "bz2" in filename:
        return bz2.open(filename, mode + "t")
    if "gz" in filename:
        return gzip.open(filename, mode + "t")
    return open(filename, mode)


def main():
    # argument parsing
    parser = argparse.ArgumentParser(description='T-Labs Video Parser v0.1',
                                     epilog="2017",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('input', type=str, help="input video")
    parser.add_argument('--dll', type=str, default="../VideoParser/libvideoparser.so", help="Path to DLL")
    parser.add_argument('--output', type=str, default="stats.json.bz2", help="Path to output JSON stats file")

    argsdict = vars(parser.parse_args())

    video_parser = videoparser.VideoParser(argsdict['input'], argsdict['dll'])
    video_parser.set_frame_callback(frame_parsed)
    video_parser.parse()

    # write stats
    if argsdict['output']:
        print("Writing stats to output file: " + argsdict['output'])
        stats = video_parser.get_stats()
        with file_open(argsdict['output'], "w") as outfile:
            json.dump(stats, outfile, indent=4, sort_keys=True)


def frame_parsed(frame_info):
    pass


if __name__ == '__main__':
    main()
