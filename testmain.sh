#!/bin/bash
scriptPath=${0%/*}

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"$scriptPath/Outputs/lib":"$scriptPath/VideoParser"
gdb -ex=run --arg "$scriptPath/VideoParser/videoparser_testmain" "$@"
#valgrind --leak-check=yes
