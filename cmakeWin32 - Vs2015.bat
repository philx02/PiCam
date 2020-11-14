set REPODIR=%CD%

set BOOST_LIBRARYDIR=%BOOST_ROOT%\lib32-msvc-14.0

mkdir "%HOMEDRIVE%\%HOMEPATH%\builds\PiCam"
pushd "%HOMEDRIVE%\%HOMEPATH%\builds\PiCam"
cmake -G "Visual Studio 14" %REPODIR%
popd

