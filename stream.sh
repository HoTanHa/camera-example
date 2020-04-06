#!/bin/sh
# arg1 is minutes and arg2 is format output.avi for instance
streamer -t 0:$1:0 -c /dev/video0 -f rgb24 -r 3  -o  $2
