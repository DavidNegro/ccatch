// -----------------------------------------------------------------
// ==== Public interface ====
// -----------------------------------------------------------------

// NOTE: this library requires the types:
// int64_t, uint64_t, size_t
#ifndef UNIT_NO_STDINT
#include <stdint.h>
#endif

// NOTE: we need strlen, memcpy, memset.
// you can use the macros UNIT_STRLEN, UNIT_MEMCPY, UNIT_MEMSET
// to override the default behaviour
#ifndef UNIT_NO_STRING_H
#include <string.h>
#endif

#ifndef UNIT_STRLEN
#define UNIT_STRLEN(x) strlen(x)
#endif

#define __TEST_EXPAND__(x,y) x y
#define __TEST_CONCAT__(x, y) x ## y
#define __TEST_STR__(x) #x

// redefine this macro to rename the symbols created by this library
#ifndef UNIT_NAME
#define UNIT_NAME(x) __test_ ## x ## __
#endif

// redefine this macro to redefine how this library generates temporal symbols
#ifndef UNIT_TMPNAME
#define UNIT_TMPNAME(x) __TEST_EXPAND__(__TEST_CONCAT__, (__test_ ## x ## __, __LINE__))
#endif

// -----------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4255)//, justification : "Declaring functions not taking arguments as fn() instead of fn(void)")
#pragma warning(disable : 4820)//, justification : "Padding bytes added to struct")
#pragma warning(disable : 5045)//, justification : "Spectre mitigation")
#endif

// -----------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------

typedef void (*UNIT_NAME(case_fn))(void);
typedef void (*UNIT_NAME(defer_fn))(void* ptr);
typedef struct UNIT_NAME(log_buffer) UNIT_NAME(log_buffer);

int UNIT_NAME(register_test)(UNIT_NAME(case_fn) fn, const char* fn_name, const char* tags);
size_t UNIT_NAME(generate)(const char* name, size_t cnt);
int UNIT_NAME(section_start)(const char* name);
int UNIT_NAME(section_end)(void);
void UNIT_NAME(defer)(void* ptr, UNIT_NAME(defer_fn) fn);
void* UNIT_NAME(scoped_alloc)(size_t size);
UNIT_NAME(log_buffer)* UNIT_NAME(log_buffer_create)(size_t line, const char* filename, const char* fmt);
void UNIT_NAME(log_buffer_log)(UNIT_NAME(log_buffer)* buff, const char* macro_name, int is_error);
int UNIT_NAME(log_buffer_add_str)(UNIT_NAME(log_buffer)* buff, const char* str, size_t sz);
int UNIT_NAME(log_buffer_add_fmt_int)(UNIT_NAME(log_buffer)* buff, int64_t v);
int UNIT_NAME(log_buffer_add_fmt_uint)(UNIT_NAME(log_buffer)* buff, uint64_t v);
int UNIT_NAME(log_buffer_add_fmt_double)(UNIT_NAME(log_buffer)* buff, double v);
void UNIT_NAME(throw)(void);
int UNIT_NAME(has_error)(void);

// -----------------------------------------------------------------
// __TEST_REGISTER_TEST__ macro
// -----------------------------------------------------------------
/* This macro uses compile-specific stuff to register tests
   automatically.
*/

#if defined(__cplusplus)
#define __TEST_REGISTER_TEST__(name, tags) struct __test_registerer_ ## name { __test_registerer_ ## name() { UNIT_NAME(register_test)(__test_ ## name, #name, tags); } }; static __test_registerer_ ## name __test_registerer_v_ ## name;
#elif defined(_MSC_VER)
// from https://stackoverflow.com/questions/1113409/attribute-constructor-equivalent-in-vc
#pragma section(".CRT$XCU",read)
#define __TEST_INITIALIZER2__(f,p) \
    static void f(void); \
    __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
    __pragma(comment(linker,"/include:" p #f "_")) \
    static void f(void)
#ifdef _WIN64
    #define __TEST_INITIALIZER__(f) __TEST_INITIALIZER2__(f,"")
#else
    #define __TEST_INITIALIZER__(f) __TEST_INITIALIZER2__(f,"_")
#endif
#define __TEST_REGISTER_TEST__(name, tags) __TEST_EXPAND__(__TEST_INITIALIZER__, (UNIT_TMPNAME(registerer_ ## name))) { UNIT_NAME(register_test)(UNIT_TMPNAME(name), #name, tags); }
#elif defined(__GNUC__)
__attribute__((constructor)) static void UNIT_TMPNAME(registerer_ ## name)() { UNIT_NAME(register_test)(UNIT_TMPNAME(name), #name, tags); }
#else
#define __TEST_REQUIRE_STATIC_INIT__
#define __TEST_REGISTER_TEST__(name, tags) void UNIT_NAME(registerer_ ## name)() { UNIT_NAME(register_test)(UNIT_TMPNAME(name), #name, tags); }
#endif

// -----------------------------------------------------------------
// Private macros
// -----------------------------------------------------------------
#ifndef UNIT_LOG_BUFFER_VARIABLE
#define UNIT_LOG_BUFFER_VARIABLE() UNIT_NAME(log_buffer_variable)
#endif

#define __TEST_LOG_BASE__(macro_name, is_error, fmt, ...)                                                          \
do {                                                                                                               \
    UNIT_NAME(log_buffer)* UNIT_NAME(log_buffer_variable) = UNIT_NAME(log_buffer_create)(__LINE__, __FILE__, fmt); \
    __VA_ARGS__;                                                                                                   \
    UNIT_NAME(log_buffer_log)(UNIT_NAME(log_buffer_variable), macro_name, is_error);                               \
} while(0)


#define __TEST_CHECKED_TYPED__(name, tp, fmt, last, x, op, y)                  \
do {                                                                           \
    tp UNIT_TMPNAME(x) = x;                                                    \
    tp UNIT_TMPNAME(y) = y;                                                    \
    if (!(UNIT_TMPNAME(x) op UNIT_TMPNAME(y))) {                               \
        __TEST_EXPAND__(__TEST_LOG_BASE__, (name, 1, "Check failed \""         \
            __TEST_STR__(x op y) "\" with " __TEST_STR__(x) " = {} "           \
            __TEST_STR__(y) " = {}", fmt(UNIT_TMPNAME(x)),                     \
            fmt(UNIT_TMPNAME(y))));                                            \
            last;                                                              \
    }                                                                          \
} while(0)

// -----------------------------------------------------------------
// Public macros
// -----------------------------------------------------------------

#define UNIT_TEST(name, tags)               \
    static void UNIT_TMPNAME(name)(void);   \
    __TEST_REGISTER_TEST__(name, tags)      \
    static void UNIT_TMPNAME(name)(void)

#define UNIT_GENERATE(name, n) UNIT_NAME(generate)(name, n)
#define UNIT_CASE(name) for (int __TEST_EXPAND__(__TEST_CONCAT__, (__test_i_, __LINE__)) = UNIT_NAME(section_start)(name); __TEST_EXPAND__(__TEST_CONCAT__, (__test_i_, __LINE__)) == 0;  __TEST_EXPAND__(__TEST_CONCAT__, (__test_i_, __LINE__)) = UNIT_NAME(section_end)())
#define UNIT_DEFER(ptr, fn) UNIT_NAME(defer)(ptr, fn)
#define UNIT_SCOPED_ALLOC(size) UNIT_NAME(scoped_alloc)(size)

#define UNIT_FMT_STR(x) UNIT_NAME(log_buffer_add_str)(UNIT_NAME(log_buffer_variable), x, UNIT_STRLEN(x))
#define UNIT_FMT_STRL(x, sz) UNIT_NAME(log_buffer_add_str)(UNIT_NAME(log_buffer_variable), x, sz)
#define UNIT_FMT_INT(x) UNIT_NAME(log_buffer_add_fmt_int)(UNIT_NAME(log_buffer_variable), x)
#define UNIT_FMT_UINT(x) UNIT_NAME(log_buffer_add_fmt_uint)(UNIT_NAME(log_buffer_variable), x)
#define UNIT_FMT_DOUBLE(x) UNIT_NAME(log_buffer_add_fmt_double)(UNIT_NAME(log_buffer_variable), x)

#define UNIT_LOG(...) __TEST_EXPAND__(__TEST_LOG_BASE__, ("LOG", 0, __VA_ARGS__))
#define UNIT_FAIL(...) __TEST_EXPAND__(__TEST_LOG_BASE__, ("FAIL", 1, __VA_ARGS__))

#define UNIT_CHECK(condition, ...) if (!(condition)) { __TEST_EXPAND__(__TEST_LOG_BASE__, ("CHECK", 1, "Check failed \""__TEST_EXPAND__(__TEST_STR__,(condition)) "\" " __VA_ARGS__)); }
#define UNIT_REQUIRE(condition, ...) if (!(condition)) { __TEST_EXPAND__(__TEST_LOG_BASE__, ("REQUIRE", 1, "Check failed \""__TEST_EXPAND__(__TEST_STR__, (condition)) "\" " __VA_ARGS__)); UNIT_NAME(throw)(); }

#define UNIT_CHECK_INT(x, op, y) __TEST_CHECKED_TYPED__("CHECK_INT", int, UNIT_FMT_INT, , x, op, y)
#define UNIT_REQUIRE_INT(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_INT", int, UNIT_FMT_INT, UNIT_NAME(throw)(), x, op, y)
#define UNIT_CHECK_UINT(x, op, y) __TEST_CHECKED_TYPED__("CHECK_UINT", unsigned, UNIT_FMT_UINT, , x, op, y)
#define UNIT_REQUIRE_UINT(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_UINT", unsigned, UNIT_FMT_UINT, UNIT_NAME(throw)(), x, op, y)
#define UNIT_CHECK_INT64(x, op, y) __TEST_CHECKED_TYPED__("CHECK_INT64", int64_t, UNIT_FMT_INT, , x, op, y)
#define UNIT_REQUIRE_INT64(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_INT64", int64_t, UNIT_FMT_INT, UNIT_NAME(throw)(), x, op, y)
#define UNIT_CHECK_UINT64(x, op, y) __TEST_CHECKED_TYPED__("CHECK_UINT64", uint64_t, UNIT_FMT_UINT, , x, op, y)
#define UNIT_REQUIRE_UINT64(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_UINT64", uint64_t, UNIT_FMT_UINT, UNIT_NAME(throw)(), x, op, y)
#define UNIT_CHECK_DOUBLE(x, op, y) __TEST_CHECKED_TYPED__("CHECK_DOUBLE", double, UNIT_FMT_DOUBLE, , x, op, y)
#define UNIT_REQUIRE_DOUBLE(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_DOUBLE", double, UNIT_FMT_DOUBLE, UNIT_NAME(throw)(), x, op, y)

#define UNIT_FINISH_IF_FAILED() if (UNIT_NAME(has_error)()) { UNIT_NAME(throw)(); }

// -----------------------------------------------------------------
// Short name macros
// -----------------------------------------------------------------

#ifndef UNIT_NO_SHORT_NAMES

#define TEST(name, tags) UNIT_TEST(name, tags)
#define GENERATE(name, n) UNIT_GENERATE(name, n)
#define CASE(name) UNIT_CASE(name)
#define DEFER(ptr, fn) UNIT_DEFER(ptr, fn)
#define SCOPED_ALLOC(size) UNIT_SCOPED_ALLOC(size)

#define FMT_STR(x) UNIT_FMT_STR(x)
#define FMT_STRL(x, sz) UNIT_FMT_STRL(x, sz)
#define FMT_INT(x) UNIT_FMT_INT(x)
#define FMT_UINT(x) UNIT_FMT_UINT(x)
#define FMT_DOUBLE(x) UNIT_FMT_DOUBLE(x)

#define LOG(...) __TEST_EXPAND__(UNIT_LOG, (__VA_ARGS__))
#define FAIL(...) __TEST_EXPAND__(UNIT_FAIL, (__VA_ARGS__))

#define CHECK(...) __TEST_EXPAND__(UNIT_CHECK, (__VA_ARGS__))
#define REQUIRE(...) __TEST_EXPAND__(UNIT_REQUIRE, (__VA_ARGS__))

#define CHECK_INT(x, op, y) UNIT_CHECK_INT(x, op, y)
#define REQUIRE_INT(x, op, y) UNIT_REQUIRE_INT(x, op, y)
#define CHECK_UINT(x, op, y) UNIT_CHECK_UINT(x, op, y)
#define REQUIRE_UINT(x, op, y) UNIT_REQUIRE_UINT(x, op, y)
#define CHECK_INT64(x, op, y) UNIT_CHECK_INT64(x, op, y)
#define REQUIRE_INT64(x, op, y) UNIT_REQUIRE_INT64(x, op, y)
#define CHECK_UINT64(x, op, y) UNIT_CHECK_UINT64(x, op, y)
#define REQUIRE_UINT64(x, op, y) UNIT_REQUIRE_UINT64(x, op, y)
#define CHECK_DOUBLE(x, op, y) UNIT_CHECK_DOUBLE(x, op, y)
#define REQUIRE_DOUBLE(x, op, y) UNIT_REQUIRE_DOUBLE(x, op, y)
#define FINISH_IF_FAILED() UNIT_FINISH_IF_FAILED()

#endif // #ifndef UNIT_NO_SHORT_NAMES
