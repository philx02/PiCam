#!/bin/bash
cd /sys/class/gpio
echo 17 > unexport
echo 18 > unexport
echo 17 > export
echo 18 > export
sleep 1
pushd gpio17 > /dev/null
echo in > direction
echo both > edge
echo 1 > active_low
popd > /dev/null
pushd gpio18 > /dev/null
echo out > direction
echo 1 > active_low
echo 0 > value
popd > /dev/null
