/* Exception throwing and handling system

    Functions:
        - void <throw>()
            stops the execution of a test case

    Macros:
        - int UNIT_RUNEXCEPT(<case_fn> test_case)
            runs the test case function and returns != 0 if the test case has
            called <throw> to abort its execution.

    Notes:
        - if UNIT_RUNEXCEPT is already defined, the implementation MUST also
          define <throw> to implemet its own exception throwing and handling
          mechanism.

*/
#ifndef UNIT_RUNEXCEPT
#ifdef __cplusplus
// for cpp use exceptions
namespace {
    struct UNIT_NAME(exception) {
    };
}

void UNIT_NAME(throw)() {
    throw UNIT_NAME(exception){};
}

static int UNIT_NAME(runexcept)(UNIT_NAME(case_fn) fn) {
    try {
        fn();
    }
    catch(UNIT_NAME(exception)) {
        return -1;
    }
    return 0;
}
#define UNIT_RUNEXCEPT(x) UNIT_NAME(runexcept)(x)
#else
// use setjmp in other cases
#include <setjmp.h>

static jmp_buf UNIT_NAME(jmp_buff);

void UNIT_NAME(throw)() {
    longjmp(UNIT_NAME(jmp_buff), -1);
}

static int UNIT_NAME(runexcept)(UNIT_NAME(case_fn) fn) {
    if (setjmp(UNIT_NAME(jmp_buff)) != -1) {
        fn();
        return 0;
    }
    return -1;
}
#define UNIT_RUNEXCEPT(x) UNIT_NAME(runexcept)(x)
#endif
#endif
