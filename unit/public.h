// -----------------------------------------------------------------
// ==== Public interface ====
// -----------------------------------------------------------------

#define __TEST_EXPAND__(x,y) x y
#define __TEST_CONCAT__(x, y) x ## y
#define __TEST_STR__(x) #x

// redefine this macro to rename the public symbols created by this library
#ifndef UNIT_NAME
#define UNIT_NAME(x) __test_ ## x ## __
#endif

// redefine this macro to redefine how this library generates temporal symbols
#ifndef UNIT_TMPNAME
#define UNIT_TMPNAME(x) __TEST_EXPAND__(__TEST_CONCAT__, (__test_ ## x ## __, __LINE__))
#endif

// define this macro for non-test builds if you are including tests in the same
// file as your code
#ifdef UNIT_NO_TEST
#define UNIT_FUNCTION_DECL static
#else
#define UNIT_FUNCTION_DECL
#endif

// -----------------------------------------------------------------
// Basic integer type definitions
// -----------------------------------------------------------------

// long long is required to have at least 64 bits in c99.
// they are 64 bits in most c compilers
typedef long long UNIT_NAME(long_int);
typedef unsigned long long UNIT_NAME(long_uint);

// msvc unint_size_t definition
#if defined(_WIN32) || defined(_WIN64)
#ifdef _WIN64
typedef unsigned long long UNIT_NAME(unit_size);
#else
typedef unsigned long UNIT_NAME(unit_size);
#endif

#else // assume 64 bit in other platforms. TODO: do not assume it
typedef unsigned long long UNIT_NAME(unit_size);
#endif


// -----------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------

typedef void (*UNIT_NAME(case_fn))(void);
typedef void (*UNIT_NAME(defer_fn))(void* ptr);
typedef struct UNIT_NAME(log_buffer) UNIT_NAME(log_buffer);

UNIT_FUNCTION_DECL UNIT_NAME(unit_size) UNIT_NAME(unit_strlen)(const char* str);
UNIT_FUNCTION_DECL int UNIT_NAME(register_test)(UNIT_NAME(case_fn) fn, const char* fn_name, const char* tags, UNIT_NAME(unit_size) line, const char* filename);
UNIT_FUNCTION_DECL UNIT_NAME(unit_size) UNIT_NAME(generate)(const char* name, UNIT_NAME(unit_size) cnt);
UNIT_FUNCTION_DECL int UNIT_NAME(section_start)(const char* name);
UNIT_FUNCTION_DECL int UNIT_NAME(section_end)(void);
UNIT_FUNCTION_DECL void UNIT_NAME(defer)(void* ptr, UNIT_NAME(defer_fn) fn);
UNIT_FUNCTION_DECL void* UNIT_NAME(scoped_alloc)(UNIT_NAME(unit_size) size);
UNIT_FUNCTION_DECL UNIT_NAME(log_buffer)* UNIT_NAME(log_buffer_create)(UNIT_NAME(unit_size) line, const char* filename, const char* fmt);
UNIT_FUNCTION_DECL void UNIT_NAME(log_buffer_log)(UNIT_NAME(log_buffer)* buff, const char* macro_name, int is_error);
UNIT_FUNCTION_DECL int UNIT_NAME(log_buffer_add_str)(UNIT_NAME(log_buffer)* buff, const char* str, UNIT_NAME(unit_size) sz);
UNIT_FUNCTION_DECL int UNIT_NAME(log_buffer_add_fmt_int)(UNIT_NAME(log_buffer)* buff, UNIT_NAME(long_int) v);
UNIT_FUNCTION_DECL int UNIT_NAME(log_buffer_add_fmt_uint)(UNIT_NAME(log_buffer)* buff, UNIT_NAME(long_uint) v);
UNIT_FUNCTION_DECL int UNIT_NAME(log_buffer_add_fmt_double)(UNIT_NAME(log_buffer)* buff, double v);
UNIT_FUNCTION_DECL void UNIT_NAME(throw)(void);
UNIT_FUNCTION_DECL int UNIT_NAME(has_error)(void);

// -----------------------------------------------------------------
// __TEST_REGISTER_TEST__ macro
// -----------------------------------------------------------------
/* This macro uses compiler-specific stuff to register tests
   automatically.
*/

#ifndef UNIT_NO_TEST
#if defined(__cplusplus)
#define __TEST_REGISTER_TEST__(name, tags) struct __test_registerer_ ## name { __test_registerer_ ## name() { UNIT_NAME(register_test)(__test_ ## name, #name, tags, __LINE__, __FILE__); } }; static __test_registerer_ ## name __test_registerer_v_ ## name;
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
#define __TEST_REGISTER_TEST__(name, tags) __TEST_EXPAND__(__TEST_INITIALIZER__, (UNIT_TMPNAME(registerer_ ## name))) { UNIT_NAME(register_test)(UNIT_TMPNAME(name), #name, tags, __LINE__, __FILE__); }
#elif defined(__GNUC__) || defined(__CLANG__) || defined(__TINYC__)
__attribute__((constructor)) static void UNIT_TMPNAME(registerer_ ## name)() { UNIT_NAME(register_test)(UNIT_TMPNAME(name), #name, tags); }
#else
#define __TEST_REQUIRE_STATIC_INIT__
#define __TEST_REGISTER_TEST__(name, tags) void UNIT_NAME(registerer_ ## name)() { UNIT_NAME(register_test)(UNIT_TMPNAME(name), #name, tags); }
#endif
#else
#define __TEST_REGISTER_TEST__(name, tags)
#endif // #ifndef UNIT_NO_TEST

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
    tp UNIT_TMPNAME(__x) = x;                                                  \
    tp UNIT_TMPNAME(__y) = y;                                                  \
    if (!(UNIT_TMPNAME(__x) op UNIT_TMPNAME(__y))) {                           \
        __TEST_EXPAND__(__TEST_LOG_BASE__, (name, 1, "Check failed \""         \
            __TEST_STR__(x op y) "\" with " __TEST_STR__(x) " = {} "           \
            __TEST_STR__(y) " = {}", fmt(UNIT_TMPNAME(__x)),                   \
            fmt(UNIT_TMPNAME(__y))));                                          \
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

#define UNIT_FMT_STR(x) UNIT_NAME(log_buffer_add_str)(UNIT_NAME(log_buffer_variable), x, UNIT_NAME(unit_strlen)(x))
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

// -----------------------------------------------------------------
// Empty static implementations (when UNIT_NO_TEST is defined)
// -----------------------------------------------------------------

#ifdef UNIT_NO_TEST

#if defined(_MSC_VER)
#define __EMPTY_BODY__(x) { __assume(0); return x; }
#elif defined(__GCC__)
#define __EMPTY_BODY__(x) { __builtin_unreachable(); return x; }
#else
#define __EMPTY_BODY__(x) { return x; }
#endif

UNIT_FUNCTION_DECL UNIT_NAME(unit_size) UNIT_NAME(unit_strlen)(const char* str) __EMPTY_BODY__(0);
UNIT_FUNCTION_DECL int UNIT_NAME(register_test)(UNIT_NAME(case_fn) fn, const char* fn_name, const char* tags) __EMPTY_BODY__(0)
UNIT_FUNCTION_DECL UNIT_NAME(unit_size) UNIT_NAME(generate)(const char* name, UNIT_NAME(unit_size) cnt) __EMPTY_BODY__(0)
UNIT_FUNCTION_DECL int UNIT_NAME(section_start)(const char* name) __EMPTY_BODY__(0)
UNIT_FUNCTION_DECL int UNIT_NAME(section_end)(void) __EMPTY_BODY__(0)
UNIT_FUNCTION_DECL void UNIT_NAME(defer)(void* ptr, UNIT_NAME(defer_fn) fn)__EMPTY_BODY__( )
UNIT_FUNCTION_DECL void* UNIT_NAME(scoped_alloc)(UNIT_NAME(unit_size) size) __EMPTY_BODY__((void*)0)
UNIT_FUNCTION_DECL UNIT_NAME(log_buffer)* UNIT_NAME(log_buffer_create)(UNIT_NAME(unit_size) line, const char* filename, const char* fmt) __EMPTY_BODY__((UNIT_NAME(log_buffer)*)0)
UNIT_FUNCTION_DECL void UNIT_NAME(log_buffer_log)(UNIT_NAME(log_buffer)* buff, const char* macro_name, int is_error) __EMPTY_BODY__( )
UNIT_FUNCTION_DECL int UNIT_NAME(log_buffer_add_str)(UNIT_NAME(log_buffer)* buff, const char* str, UNIT_NAME(unit_size) sz) __EMPTY_BODY__(0)
UNIT_FUNCTION_DECL int UNIT_NAME(log_buffer_add_fmt_int)(UNIT_NAME(log_buffer)* buff, UNIT_NAME(long_int) v) __EMPTY_BODY__(0)
UNIT_FUNCTION_DECL int UNIT_NAME(log_buffer_add_fmt_uint)(UNIT_NAME(log_buffer)* buff, UNIT_NAME(long_uint) v) __EMPTY_BODY__(0)
UNIT_FUNCTION_DECL int UNIT_NAME(log_buffer_add_fmt_double)(UNIT_NAME(log_buffer)* buff, double v) __EMPTY_BODY__(0)
UNIT_FUNCTION_DECL void UNIT_NAME(throw)(void) __EMPTY_BODY__( )
UNIT_FUNCTION_DECL int UNIT_NAME(has_error)(void) __EMPTY_BODY__(0)

#undef UNIT_FUNCTION_DECL
#undef __EMPTY_BODY__

#endif // #ifdef UNIT_NO_TEST
