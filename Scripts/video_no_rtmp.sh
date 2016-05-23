#!/bin/bash
/home/pi/PiCam/raspivid -t 0 -hf -vf -sg 3600000 -wr 168 -sn 0 -fps 25 -w 1280 -h 720 -b 500000 -o /home/pi/PiCam/data/video%04d.h264 | cvlc -vvv stream:///dev/stdin --sout '#standard{access=http,mux=ts,dst=:8090}' :demux=h264
