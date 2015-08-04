set REPODIR=%CD%

mkdir "%HOMEDRIVE%\%HOMEPATH%\builds\PiCam"
pushd "%HOMEDRIVE%\%HOMEPATH%\builds\PiCam"
cmake -G "Visual Studio 12" %REPODIR%
popd

