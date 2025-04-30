@echo off
pushd "%~dp0"
call build.bat

rem --- compile ---
cl tests\unit_test.c /I release /I unit_platforms /P
popd
