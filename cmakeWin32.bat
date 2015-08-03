set REPODIR=%CD%

mkdir %HOMEDRIVE%\%HOMEPATH%\builds\PiCam
pushd %HOMEDRIVE%\%HOMEPATH%\builds\PiCam
cmake -G "Visual Studio 11" %REPODIR%
popd

