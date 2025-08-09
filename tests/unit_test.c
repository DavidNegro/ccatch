#include "unit.h"
#include <stddef.h>

#ifdef _MSC_VER
// no spectre mitigation warnings
#pragma warning(disable : 5045)
#endif

TEST(MultipleCases, "[.tags-are-not-implemented-yet][tag1][tag2]") {
    LOG("Prepare ...");

    CASE("Test1") {
        LOG("Hello from {}", FMT_STR("test1"));

        CASE("Test1-1") {
            LOG("Hello from {}", FMT_DOUBLE(1.1));
        }

        CASE("Test1-2") {
            LOG("Hello from {}", FMT_STR("1-2"));
        }
    }

    CASE("Test2") {
        LOG("Hello from {}", FMT_STR("test2"));
        size_t i = GENERATE("Test generate", 2);
        LOG("Hello from {} generated case {}", FMT_STR("test2"), FMT_UINT(i));
    }
    LOG("End");
}


TEST(Assertions, "[tag][!shouldfail]") {
    int a = 8;
    int b = 16;

    CASE("Checks") {
        CHECK(a < b);
        CHECK(a > b, "a is not bigger than b with a = {} b = {}", FMT_INT(a), FMT_INT(b));
        // test will continue even if the CHECK condition fails (use require to
        // finish the excution if the condition is not meet)
        CHECK_INT(a, >, b);
    }

    CASE("Require") {
        // If we don't use a generate here, the test may not detect the cases not
        // executed after the exception (TODO add a function to specify the number
        // of cases we have in a gives scope so we can avoid this issue when it is
        // annoying
        size_t c = GENERATE("Use simple require", 2);
        if (c == 0) {
            REQUIRE(a > b, "a is not bigger than b with a = {} b = {}", FMT_INT(a), FMT_INT(b));
            LOG("This line will not be executed");
        }
        else {
            REQUIRE_INT(a, >, b);
            LOG("This line will not be executed");
        }
    }

}

TEST(ScopedAllocations, "[tag]") {
    void* memory = SCOPED_ALLOC(1024);
    (void)memory; // ignore unused variable warning;
    GENERATE("Repeat this many times to make sure the allocation don't leak ", 400);
}

static void DeferExecution1(void* p) {
    (void)p; // ignore unused variable warning;
    LOG("Defer 1");
}

static void DeferExecution2(void* p) {
    (void)p; // ignore unused variable warning;
    LOG("Defer 2");
}

static void DeferExecution3(void* p) {
    (void)p; // ignore unused variable warning;
    LOG("Defer 3");
}

TEST(Defer, "[tag]") {
    DEFER(0, DeferExecution1);
    DEFER(0, DeferExecution2);
    DEFER(0, DeferExecution3);
    LOG("Test Ended");
    // defer will be executed in inverese order after the test finished
}
