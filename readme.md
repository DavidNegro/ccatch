# Experimental Catch2-like Test Framework for C

This is a header-only unit testing framework for C, inspired by Catch2.

## IMPORTANT

I built it just for fun, it is experimental — **DO NOT USE**.

See `unit_test.c` & `unit_impl.c` as an usage example.

Only tested in Windows with msvc.

To create the amalgamated header, run a developer command prompt (or run the
`scripts/developer_cmd.ps1` prompt script) and run `scripts/build.bat`

`xmake` support is also experimental. I'm mainly using it for release generation. Run `xmake publish_release` to generate a github release
