export REPODIR=$PWD

mkdir -p "$HOME/builds/PiCam"
pushd "$HOME/builds/PiCam"
cmake -DCMAKE_TOOLCHAIN_FILE=Toolchain-gcc-linaro-arm.cmake $REPODIR
popd
