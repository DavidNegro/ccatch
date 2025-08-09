@echo off
pushd "%~dp0\.."

rem Create directories
if not exist "release" mkdir release
if not exist "tmp" mkdir tmp

rem Compile amalgamate tool
cl unit\amalgamate.c /Wall /WX /Fe:tmp\amalgamate.exe /Fd:tmp\amalgamate.pdb /Fo:tmp\amalgamate.obj

rem Run amalgamate tool
.\tmp\amalgamate.exe release\unit.h

popd
