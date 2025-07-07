#!/bin/bash
scriptPath=${0%/*}

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"$scriptPath/Outputs/lib"
#gdb -ex=run --arg
#gdb --args
python3 "$scriptPath/PythonInterface/parse.py" --dll "$scriptPath/VideoParser/libvideoparser.so" "$@"
