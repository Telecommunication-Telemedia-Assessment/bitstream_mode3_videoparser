#!/bin/bash

echo "run some tests"

for video in test_videos/*.mkv; do
    ./parser.sh "$video" --output "$(basename "$video" .mkv).json.bz2"
done
