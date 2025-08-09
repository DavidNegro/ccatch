@echo off
pushd "%~dp0\.."
call .\scripts\build.bat

rem --- compile ---
cl tests\unit_test_impl.c tests\unit_test.c /I release /I unit_platforms /Wall /WX /Fo:tmp\ /Fe:tmp\unit_test.exe /Fd:tmp\unit_test.pdb
.\tmp\unit_test.exe
popd
