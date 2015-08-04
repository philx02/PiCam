set REPODIR=%CD%

set BOOST_ROOT=%BOOST_ROOT_VS2013%

mkdir "%HOMEDRIVE%\%HOMEPATH%\builds\PiCam"
pushd "%HOMEDRIVE%\%HOMEPATH%\builds\PiCam"
cmake -G "Visual Studio 12" %REPODIR%
popd

