/** TODO: this code needs a refactor
 *  TODO: support command line args
*/

#pragma once

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
#endif

void UNIT_NAME(writeoutput)(int endln, const char* data, size_t size) {
    static void* hdl = 0;
    if (hdl == 0) {
        hdl = GetStdHandle((unsigned long)-11);
    }
    WriteConsoleA(hdl, data, (unsigned long)size, 0, 0);
    if (endln) {
        WriteConsoleA(hdl, UNIT_ENDLINE, 2, 0, 0);
    }
}

#define UNIT_WRITEOUTPUT(endln, str, len) UNIT_NAME(writeoutput)(endln, str, len);
#else
#error "TODO"
#endif // #ifdef _WIN32
#endif //#ifndef UNIT_WRITEOUTPUT

// -------------------------------------------------------------------

#define UNIT_WRITEOUTPUTS(endln, str) UNIT_WRITEOUTPUT(endln, str, UNIT_STRLEN(str))

static int UNIT_NAME(platform_on_log)(UNIT_NAME(log_buffer)* buff, const char* macro_name, int is_error) {
    UNIT_WRITEOUTPUTS(0, "  ");
    char TMP[22];
    TMP[21] = ' ';
    size_t sz = UNIT_NAME(back_push_uint_unsafe)(TMP + 21, buff->line);
    TMP[21 - sz - 1] = ':';
    if (is_error) {
        UNIT_WRITEOUTPUTS(0, UNIT_MAGENTA);
        UNIT_WRITEOUTPUTS(0, macro_name);
        UNIT_WRITEOUTPUTS(0, UNIT_COLOUR_END);
    }
    else {
        UNIT_WRITEOUTPUTS(0, UNIT_BLUE);
        UNIT_WRITEOUTPUTS(0, macro_name);
        UNIT_WRITEOUTPUTS(0, UNIT_COLOUR_END);
    }
    UNIT_WRITEOUTPUTS(0, " ");
    UNIT_WRITEOUTPUTS(0, UNIT_BLACK)
    UNIT_WRITEOUTPUTS(0, buff->filename);
    UNIT_WRITEOUTPUT(0, TMP + (21 - sz - 1), sz + 2);
    UNIT_WRITEOUTPUTS(0, UNIT_COLOUR_END);
    UNIT_WRITEOUTPUT(1, buff->buff.memory, buff->buff.size);
    UNIT_NAME(log_buffer_destroy)(buff);
    return 0;
}

static int UNIT_NAME(platform_on_subtest_start)(struct UNIT_NAME(test_registry)* test, struct UNIT_NAME(flow_path)* subtest_path, size_t subtest_path_length) {
    return 0;
}

static int UNIT_NAME(platform_on_subtest_finished)(struct UNIT_NAME(test_registry)* test, struct UNIT_NAME(flow_path)* subtest_path, size_t subtest_path_length, int with_error) {
    if (with_error) {
        UNIT_WRITEOUTPUTS(1, "  Failed subcase:");
        for (size_t i = 0; i < subtest_path_length; i++) {
            UNIT_NAME(flow_path)* p = subtest_path + i;
            UNIT_WRITEOUTPUTS(0, UNIT_BLACK);
            if (p->flags & UNIT_NAME(flow_flag_is_section)) {
                UNIT_WRITEOUTPUTS(0, "    CASE: ");
            }
            else {
                UNIT_WRITEOUTPUTS(0, "    GENERATE ");
                char TMP[22];
                TMP[20] = ':';
                TMP[21] = ' ';
                size_t sz = UNIT_NAME(back_push_uint_unsafe)(TMP + 20, p->index);
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

int main() {
    int result = 0;
    UNIT_NAME(init)();
    size_t num_tests;
    const UNIT_NAME(test_registry)* tests = UNIT_NAME(get_test_registry)(&num_tests);
    for (size_t i = 0; i < num_tests; i++) {
        UNIT_NAME(test_registry)* reg = tests + i;
        UNIT_WRITEOUTPUTS(1, "");
        UNIT_WRITEOUTPUTS(0, UNIT_BOLD UNIT_CYAN "> " UNIT_COLOUR_END UNIT_BOLD_END "Running test " UNIT_BOLD UNIT_BRIGHT_YELLOW);
        UNIT_WRITEOUTPUTS(0, reg->name);
        UNIT_WRITEOUTPUTS(1, UNIT_COLOUR_END UNIT_BOLD_END);

        int test_result = UNIT_NAME(test_runner_run_test_all_subcases)(reg);
        char TMP[20];
        // UNIT_WRITEOUTPUT(1, "", 0);
        // UNIT_WRITEOUTPUT(1, "Test results:", 13);
        // UNIT_WRITEOUTPUT(0, "    Failed cases: ", 18);
        // size_t num_size = __test_back_push_uint_unsafe__(TMP + 20, num_failed_cases);
        // UNIT_WRITEOUTPUT(1, TMP + 20 - num_size, num_size);
        // UNIT_WRITEOUTPUT(0, "    Total cases: ", 17);
        // num_size = __test_back_push_uint_unsafe__(TMP + 20, num_cases);
        // UNIT_WRITEOUTPUT(1, TMP + 20 - num_size, num_size);
        //UNIT_WRITEOUTPUT(1, "", 0);
        if (test_result) {
            UNIT_WRITEOUTPUTS(1, UNIT_BOLD UNIT_BRIGHT_RED  "FAILED" UNIT_COLOUR_END UNIT_BOLD_END);
        }
        else {
            UNIT_WRITEOUTPUTS(1, UNIT_BOLD UNIT_BRIGHT_GREEN "SUCCEEDED" UNIT_COLOUR_END UNIT_BOLD_END);
        }        result |= test_result;
    }
    UNIT_NAME(deinit)();
    return result;
}
