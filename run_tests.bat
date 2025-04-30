@echo off
pushd "%~dp0"
call build.bat

rem --- compile ---
cl tests\unit_impl.c tests\unit_test.c /I release /I unit_platforms /Wall /Fo:tmp\ /Fe:tmp\unit_test.exe /Fd:tmp\unit_test.pdb
.\tmp\unit_test.exe
popd
