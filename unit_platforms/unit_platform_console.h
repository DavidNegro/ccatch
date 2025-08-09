/** TODO: this code needs a refactor
 *  TODO: support command line args
*/

#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 5045)//, justification : "Spectre mitigation")
#endif

#ifndef UNIT_IMPL
#error Invalid include!. This file must be included after defining UNIT_IMPL
#endif

#ifndef __UNIT_INCLUDE_H__
#error Invalid include!. This file must be included after including "unit.h"
#endif

// -------------------------------------------------------------------

#define UNIT_BLACK "\033[38;5;0m"
#define UNIT_BLUE "\033[38;5;4m"
#define UNIT_GREEN "\033[38;5;2m"
#define UNIT_BRIGHT_GREEN "\033[38;5;10m"
#define UNIT_RED "\033[38;5;1m"
#define UNIT_BRIGHT_RED "\033[38;5;9m"
#define UNIT_YELLOW "\033[38;5;3m"
#define UNIT_BRIGHT_YELLOW "\033[38;5;11m"
#define UNIT_CYAN "\033[38;5;6m"
#define UNIT_MAGENTA "\033[38;5;5m"
#define UNIT_BRIGHT_MAGENTA "\033[38;5;13m"
#define UNIT_COLOUR_END "\033[39m"

#define UNIT_BOLD "\033[1m"
#define UNIT_BOLD_END "\033[0m"

// -------------------------------------------------------------------

#ifndef UNIT_WRITEOUTPUT
#ifdef _WIN32

#define UNIT_ENDLINE "\r\n"

#ifndef _PROCESSENV_ // avoid including all the windows.h header
__declspec( dllimport )
void* __stdcall GetStdHandle(unsigned long nStdHandle);
#endif

#ifndef _APISETCONSOLE_
__declspec( dllimport )
int __stdcall WriteConsoleA(
    void* hConsoleOutput,
    const void* lpBuffer,
    unsigned long nNumberOfCharsToWrite,
    unsigned long* lpNumberOfCharsWritten,
    void* lpReserved
);

__declspec( dllimport )
int __stdcall GetConsoleMode(
  void*  hConsoleHandle,
  unsigned long* lpMode
);

#endif

void unit_writeoutput(int endln, const char* data, size_t size) {
    static void* hdl = 0;
    if (hdl == 0) {
        hdl = GetStdHandle((unsigned long)-11);
    }
    unsigned long ignored;
    if (GetConsoleMode(hdl, &ignored)) {
        WriteConsoleA(hdl, data, (unsigned long)size, 0, 0);
        if (endln) {
            WriteConsoleA(hdl, UNIT_ENDLINE, 2, 0, 0);
        }
    }
    else {
        // use WriteFile
        // TODO
        UNIT_ABORT();
    }
}

#define UNIT_WRITEOUTPUT(endln, str, len) unit_writeoutput(endln, str, len);
#else
#error "TODO"
#endif // #ifdef _WIN32
#endif //#ifndef UNIT_WRITEOUTPUT

// -------------------------------------------------------------------

#define UNIT_WRITEOUTPUTS(endln, str) UNIT_WRITEOUTPUT(endln, str, UNIT_STRLEN(str))

static int unit_platform_on_log(unit_log_buffer_t* buff, const char* macro_name, int is_error) {
    UNIT_WRITEOUTPUTS(0, "  ");
    char TMP[22];
    TMP[21] = ' ';
    size_t sz = _back_push_uint_unsafe(TMP + 21, buff->line);
    TMP[21 - sz - 1] = ':';
    if (is_error) {
        UNIT_WRITEOUTPUTS(0, UNIT_MAGENTA);
    }
    else {
        UNIT_WRITEOUTPUTS(0, UNIT_BLUE);
    }
    UNIT_WRITEOUTPUTS(0, macro_name);
    UNIT_WRITEOUTPUTS(0, UNIT_COLOUR_END);

    UNIT_WRITEOUTPUTS(0, " ");
    UNIT_WRITEOUTPUTS(0, UNIT_BLACK)
    UNIT_WRITEOUTPUTS(0, buff->filename);
    UNIT_WRITEOUTPUT(0, TMP + (21 - sz - 1), sz + 2);
    UNIT_WRITEOUTPUTS(0, UNIT_COLOUR_END);
    UNIT_WRITEOUTPUT(1, buff->buff.memory, buff->buff.size);
    unit_log_buffer_destroy(buff);
    return 0;
}

static int unit_platform_on_subtest_start(const unit_test_registry_t* test, unit_flow_path_t* subtest_path, unit_size_t subtest_path_length) {
    UNIT_IGNORE_UNUSED(test);
    UNIT_IGNORE_UNUSED(subtest_path);
    UNIT_IGNORE_UNUSED(subtest_path_length);
    return 0;
}

static int unit_platform_on_subtest_finished(const unit_test_registry_t* test, unit_flow_path_t* subtest_path, unit_size_t subtest_path_length, int with_error) {
    UNIT_IGNORE_UNUSED(test);
    if (with_error) {
        UNIT_WRITEOUTPUTS(1, "  Failed subcase:");
        for (size_t i = 0; i < subtest_path_length; i++) {
            unit_flow_path_t* p = subtest_path + i;
            UNIT_WRITEOUTPUTS(0, UNIT_BLACK);
            if (p->flags & unit_flow_flag_is_section) {
                UNIT_WRITEOUTPUTS(0, "    CASE: ");
            }
            else {
                UNIT_WRITEOUTPUTS(0, "    GENERATE ");
                char TMP[22];
                TMP[20] = ':';
                TMP[21] = ' ';
                size_t sz = _back_push_uint_unsafe(TMP + 20, p->index);
                UNIT_WRITEOUTPUT(0, TMP + 20 - sz, sz + 2);
            }
            UNIT_WRITEOUTPUTS(0, UNIT_COLOUR_END UNIT_BRIGHT_YELLOW);
            UNIT_WRITEOUTPUT(1, p->description, UNIT_STRLEN(p->description));
            UNIT_WRITEOUTPUTS(0, UNIT_COLOUR_END);
        }
    }
    return 0;
}

// -------------------------------------------------------------------
// Main
// -------------------------------------------------------------------

int main(int argc, char *argv[]) {

    // parse filter
    int use_filter = 0;
    unit_test_specs_t filter;

    if (argc >= 2) {
        use_filter = 1;
        if (unit_test_specs_init(&filter, argv[1], argv[1] + UNIT_STRLEN(argv[1])) < 0) {
            UNIT_WRITEOUTPUTS(0, UNIT_BOLD UNIT_RED "INVALID COMMAND LINE PARAMETER: "  UNIT_COLOUR_END UNIT_BOLD_END);
            UNIT_WRITEOUTPUTS(0, "Error parsing test specs filter string: \"");
            UNIT_WRITEOUTPUTS(0, argv[1]);
            UNIT_WRITEOUTPUTS(1, "\"");
            UNIT_WRITEOUTPUTS(1, unit_test_specs_get_parse_error_description(argv[1]));
            return -1;
        }
    }

    int result = 0;
    unit_init();
    unit_size_t num_tests;
    const unit_test_registry_t* tests = unit_get_test_registry(&num_tests);
    for (size_t i = 0; i < num_tests; i++) {
        const unit_test_registry_t* reg = tests + i;

        int should_skip_test = 0;
        if (use_filter) {
            should_skip_test = !unit_test_specs_match(&filter, reg);
        }
        else {
            should_skip_test = reg->flags & unit_test_flag_exclude_from_default;
        }

        if (should_skip_test) {
            // TODO: maybe log test skipped
            continue;
        }

        UNIT_WRITEOUTPUTS(1, "");
        UNIT_WRITEOUTPUTS(0, UNIT_BOLD UNIT_CYAN "> " UNIT_COLOUR_END UNIT_BOLD_END "Running test " UNIT_BOLD UNIT_BRIGHT_YELLOW);
        UNIT_WRITEOUTPUTS(0, reg->name);
        UNIT_WRITEOUTPUTS(1, UNIT_COLOUR_END UNIT_BOLD_END);

        int test_result = unit_test_runner_run_test_all_subcases(reg);

        if (reg->flags & unit_test_flag_should_fail) {
            if (test_result) {
                UNIT_WRITEOUTPUTS(0, UNIT_BOLD UNIT_BRIGHT_GREEN "FAILED"  UNIT_COLOUR_END UNIT_BOLD_END " (expected failure)");
                test_result = 0;
            }
            else {
                UNIT_WRITEOUTPUTS(0, UNIT_BOLD UNIT_BRIGHT_RED  "SUCCEEDED" UNIT_COLOUR_END UNIT_BOLD_END " (expected failure)");
                test_result = 1;
            }
        }
        else {
            if (test_result) {
                UNIT_WRITEOUTPUTS(0, UNIT_BOLD UNIT_BRIGHT_RED  "FAILED" UNIT_COLOUR_END UNIT_BOLD_END);
            }
            else {
                UNIT_WRITEOUTPUTS(0, UNIT_BOLD UNIT_BRIGHT_GREEN "SUCCEEDED" UNIT_COLOUR_END UNIT_BOLD_END);
            }
        }
        // don't take this test result into the final calculation
        if (test_result && (reg->flags & unit_test_flag_may_fail)) {
            UNIT_WRITEOUTPUTS(1, " (ignored)");
        }
        else {
            result |= test_result;
            UNIT_WRITEOUTPUTS(1, "");
        }
    }
    unit_deinit();
    return result;
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif
