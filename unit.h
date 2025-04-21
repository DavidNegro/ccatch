#pragma once

#ifndef UNIT_NO_STDINT
#include <stdint.h>
#endif
#include <string.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4255)//, justification : "Declaring functions not taking arguments as fn() instead of fn(void)")
#pragma warning(disable : 4820)//, justification : "Padding bytes added to struct")
#pragma warning(disable : 5045)//, justification : "Spectre mitigation")
#endif

// --- forward declarations ---

typedef void (*__test_case_fn__)(void);
typedef void (*__test_defer_fn__)(void* ptr);
typedef struct __test_log_buffer__ __test_log_buffer__;

int __test_register_test__(__test_case_fn__ fn, const char* fn_name, const char* tags);
size_t __test_generate__(const char* name, size_t cnt);
int __test_section_start__(const char* name);
int __test_section_end__(void);
void __test_defer__(void* ptr, __test_defer_fn__ fn);
void* __test_scoped_alloc__(size_t size);
__test_log_buffer__* __test_log_buffer_create__(size_t line, const char* filename, const char* fmt);
void __test_log_buffer_log__(__test_log_buffer__* buff, const char* macro_name, int is_error);
void __test_log_buffer_add_str__(__test_log_buffer__* buff, const char* str, size_t sz);
void __test_log_buffer_add_fmt_int__(__test_log_buffer__* buff, int64_t v);
void __test_log_buffer_add_fmt_uint__(__test_log_buffer__* buff, uint64_t v);
void __test_log_buffer_add_fmt_double__(__test_log_buffer__* buff, double v);
void __test_throw__(void);
int __test_has_error__(void);

// --- macros ---

#define __TEST_EXPAND__(x,y) x y
#define __TEST_CONCAT__(x, y) x ## y
#define __TEST_STR__(x) #x

#if defined(__cplusplus)
#define __TEST_REGISTER_TEST(name, tags) struct __test_registerer_ ## name { __test_registerer_ ## name() { __test_register_test__(__test_ ## name, #name, tags); } }; static __test_registerer_ ## name __test_registerer_v_ ## name;
#elif defined(_MSC_VER)
// from https://stackoverflow.com/questions/1113409/attribute-constructor-equivalent-in-vc
#pragma section(".CRT$XCU",read)
#define __TEST_INITIALIZER2_(f,p) \
    static void f(void); \
    __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
    __pragma(comment(linker,"/include:" p #f "_")) \
    static void f(void)
#ifdef _WIN64
    #define __TEST_INITIALIZER(f) __TEST_INITIALIZER2_(f,"")
#else
    #define __TEST_INITIALIZER(f) __TEST_INITIALIZER2_(f,"_")
#endif
#define __TEST_REGISTER_TEST(name, tags) __TEST_INITIALIZER(__test_registerer_ ## name) { __test_register_test__(__test_ ## name, #name, tags); }
#elif defined(__GNUC__)
__attribute__((constructor)) static void __test_registerer_ ## name() { __test_register_test__(__test_ ## name, #name, tags); }
#else
#define __TEST_REQUIRE_STATIC_INIT__
#define __TEST_REGISTER_TEST(name, tags) void __test_registerer_ ## name() { __test_register_test__(__test_ ## name, #name, tags); }
#endif

#define TEST(name, tags)                  \
    static void __test_ ## name(void);    \
    __TEST_REGISTER_TEST(name, tags)      \
    static void __test_ ## name(void)

#define GENERATE(name, n) __test_generate__(name, n)
#define CASE(name) for (int __TEST_EXPAND__(__TEST_CONCAT__, (__test_i_, __LINE__)) = __test_section_start__(name); __TEST_EXPAND__(__TEST_CONCAT__, (__test_i_, __LINE__)) == 0;  __TEST_EXPAND__(__TEST_CONCAT__, (__test_i_, __LINE__)) = __test_section_end__())

#define DEFER(ptr, fn) __test_defer__(ptr, fn)
#define SCOPED_ALLOC(size) __test_scoped_alloc__(size)

#define __TEST_LOG_BASE__(macro_name, is_error, fmt, ...)                                                         \
    do {                                                                                                          \
        __test_log_buffer__* __test_log_buffer_variable__ = __test_log_buffer_create__(__LINE__, __FILE__, fmt);  \
        __VA_ARGS__;                                                                                              \
        __test_log_buffer_log__(__test_log_buffer_variable__, macro_name " ", is_error);                              \
    } while(0);

#define FMT_STR(x) __test_log_buffer_add_str__(__test_log_buffer_variable__, x, strlen(x))
#define FMT_STRL(x, sz) __test_log_buffer_add_str__(__test_log_buffer_variable__, x, sz)
#define FMT_INT(x) __test_log_buffer_add_fmt_int__(__test_log_buffer_variable__, x)
#define FMT_UINT(x) __test_log_buffer_add_fmt_uint__(__test_log_buffer_variable__, x)
#define FMT_DOUBLE(x) __test_log_buffer_add_fmt_double__(__test_log_buffer_variable__, x)

#define LOG(...) __TEST_EXPAND__(__TEST_LOG_BASE__, ("LOG", 0, __VA_ARGS__))
#define FAIL(...) __TEST_EXPAND__(__TEST_LOG_BASE__, ("FAIL", 1, __VA_ARGS__))

#define CHECK(condition, ...) if (!(condition)) { __TEST_EXPAND__(__TEST_LOG_BASE__, ("CHECK", 1, "Check failed \""__TEST_STR__(condition) "\" " __VA_ARGS__)); }
#define REQUIRE(condition, ...) if (!(condition)) { __TEST_EXPAND__(__TEST_LOG_BASE__, ("REQUIRE", 1, "Check failed \""__TEST_STR__( condition) "\" " __VA_ARGS__)); __test_throw__(); }

#define __TEST_CHECKED_TYPED__(name, tp, fmt, last, x, op, y)                  \
do {                                                                           \
    tp __test_tmp_x__ = x;                                                     \
    tp __test_tmp_y__ = y;                                                     \
    if (!(__test_tmp_x__ op __test_tmp_y__)) {                                 \
        __TEST_EXPAND__(__TEST_LOG_BASE__, (name, 1, "Check failed \""         \
            __TEST_STR__(x op y) "\" with " __TEST_STR__(x) " = {} "           \
            __TEST_STR__(y) " = {}", fmt(__test_tmp_x__),                      \
            fmt(__test_tmp_y__)));                                             \
            last;                                                              \
    }                                                                          \
} while(0)

#define CHECK_INT(x, op, y) __TEST_CHECKED_TYPED__("CHECK_INT", int, FMT_INT, , x, op, y)
#define REQUIRE_INT(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_INT", int, FMT_INT, __test_throw__(), x, op, y)
#define CHECK_UINT(x, op, y) __TEST_CHECKED_TYPED__("CHECK_UINT", unsigned, FMT_UINT, , x, op, y)
#define REQUIRE_UINT(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_UINT", unsigned, FMT_UINT, __test_throw__(), x, op, y)
#define CHECK_INT64(x, op, y) __TEST_CHECKED_TYPED__("CHECK_INT64", int64_t, FMT_INT, , x, op, y)
#define REQUIRE_INT64(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_INT64", int64_t, FMT_INT, __test_throw__(), x, op, y)
#define CHECK_UINT64(x, op, y) __TEST_CHECKED_TYPED__("CHECK_UINT64", uint64_t, FMT_UINT, , x, op, y)
#define REQUIRE_UINT64(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_UINT64", uint64_t, FMT_UINT, __test_throw__(), x, op, y)
#define CHECK_DOUBLE(x, op, y) __TEST_CHECKED_TYPED__("CHECK_DOUBLE", double, FMT_DOUBLE, , x, op, y)
#define REQUIRE_DOUBLE(x, op, y) __TEST_CHECKED_TYPED__("REQUIRE_DOUBLE", double, FMT_DOUBLE, __test_throw__(), x, op, y)

#define FINISH_IF_FAILED() if (__test_has_error__()) { __test_throw__(); }

// MARK: - Implementation

#ifdef UNIT_IMPL

#include <stdarg.h> // va_list...
#include <stdio.h>  // puts
#include <string.h> // memcpy, memset, strlen
#include <stdlib.h> // malloc free abort

// ---- utility ----

#ifndef UNIT_ALLOC
#define UNIT_ALLOC(x) malloc(x)
#endif

#ifndef UNIT_FREE
#define UNIT_FREE(x) free(x)
#endif

#ifndef UNIT_MEMCPY
#define UNIT_MEMCPY(des, src, size) memcpy(des, src, size)
#endif

#ifndef UNIT_STRLEN
#define UNIT_STRLEN(x) strlen(x)
#endif

#ifndef UNIT_ABORT
#define UNIT_ABORT() abort()
#endif

#define __TEST_MAX(x, y) (((x) > (y))?(x):(y))

// MARK: - runexcept

#ifndef UNIT_RUNEXCEPT
#ifdef __cplusplus
namespace {
    struct __test_exception__ {
    };
}

void __test_throw__() {
    throw __test_exception__{};
}

static int __test_runexcept__(__test_case_fn__ fn) {
    try {
        fn();
    }
    catch(__test_exception__) {
        return -1;
    }
    return 0;
}
#define UNIT_RUNEXCEPT(x) __test_runexcept__(x)
#else
#include <setjmp.h>

static jmp_buf __test_jmp_buff__;

void __test_throw__(void) {
    longjmp(__test_jmp_buff__, -1);
}

static int __test_runexcept__(__test_case_fn__ fn) {
    if (setjmp(__test_jmp_buff__) != -1) {
        fn();
        return 0;
    }
    return -1;
}
#define UNIT_RUNEXCEPT(x) __test_runexcept__(x)
#endif
#endif

// MARK: - writeoutput

#ifndef UNIT_WRITEOUTPUT
#ifdef _WIN32

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

void __test_writeoutput__(int endln, const char* data, size_t size) {
    static void* hdl = 0;
    if (hdl == 0) {
        hdl = GetStdHandle((unsigned long)-11);
    }
    WriteConsoleA(hdl, data, (unsigned long)size, 0, 0);
    if (endln) {
        WriteConsoleA(hdl, "\r\n", 2, 0, 0);
    }
}

#define UNIT_WRITEOUTPUT(endln, str, len) __test_writeoutput__(endln, str, len);
#else
#error "TODO"
#endif // #ifdef _WIN32
#endif //#ifndef UNIT_WRITEOUTPUT

// MARK: - mutex

#ifndef UNIT_NO_MULTITHREAD
// TODO: mutex are not working
#ifdef _WIN32
// win32
typedef void* __test_mutex__; // equivalent to an SRWLOCK)
/*
typedef struct _RTL_SRWLOCK {
        PVOID Ptr;
} RTL_SRWLOCK, *PRTL_SRWLOCK;
*/
#define __test_mutex_init__(x) do{*(x) = 0; }while(0) // equivalent to initializing with RTL_SRWLOCK_INIT
#define __test_mutex_deinit__(x) do{}while(0)
/*
#define RTL_SRWLOCK_INIT {0}
*/
#ifndef _SYNCHAPI_H_
// avoid including all the Windows.h stuff
typedef struct _RTL_SRWLOCK RTL_SRWLOCK;
typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;

__declspec( dllimport )
void
__stdcall
ReleaseSRWLockShared(PSRWLOCK SRWLock);

__declspec( dllimport )
void
__stdcall
AcquireSRWLockExclusive(PSRWLOCK SRWLock);
#endif

#define __test_mutex_lock__(x) AcquireSRWLockExclusive((PSRWLOCK)(x))
#define __test_mutex_unlock__(x) ReleaseSRWLockShared((PSRWLOCK)(x))

#else
// pthread
#error "TODO!!!!"

#endif
#else // #ifndef UNIT_NO_MULTITHREAD
typedef struct {} __test_mutex__;
#define __test_mutex_init__(x) do{}while(0)
#define __test_mutex_lock__(x) do{}while(0)
#define __test_mutex_unlock__(x) do{}while(0)
#define __test_mutex_deinit__(x) do{}while(0)
#endif

// MARK: - fpconv

#ifndef UNIT_NO_FP
// ---- fpconv ----
// code taken and adapted from https://github.com/night-shift/fpconv/
// commit: 066e1d34602263adca78e13faee6db255ddcdef9
// license: Boost
/*
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4242)
#endif

// from file powers.h

#define __test_npowers__     87
#define __test_steppowers__  8
#define __test_firstpower__ -348 /* 10 ^ -348 */

#define __test_expmax__     -32
#define __test_expmin__     -60


typedef struct __test_fp__ {
    uint64_t frac;
    int exp;
} __test_fp__;

static __test_fp__ __test_powers_ten__[] = {
    { 18054884314459144840U, -1220 }, { 13451937075301367670U, -1193 },
    { 10022474136428063862U, -1166 }, { 14934650266808366570U, -1140 },
    { 11127181549972568877U, -1113 }, { 16580792590934885855U, -1087 },
    { 12353653155963782858U, -1060 }, { 18408377700990114895U, -1034 },
    { 13715310171984221708U, -1007 }, { 10218702384817765436U, -980 },
    { 15227053142812498563U, -954 },  { 11345038669416679861U, -927 },
    { 16905424996341287883U, -901 },  { 12595523146049147757U, -874 },
    { 9384396036005875287U,  -847 },  { 13983839803942852151U, -821 },
    { 10418772551374772303U, -794 },  { 15525180923007089351U, -768 },
    { 11567161174868858868U, -741 },  { 17236413322193710309U, -715 },
    { 12842128665889583758U, -688 },  { 9568131466127621947U,  -661 },
    { 14257626930069360058U, -635 },  { 10622759856335341974U, -608 },
    { 15829145694278690180U, -582 },  { 11793632577567316726U, -555 },
    { 17573882009934360870U, -529 },  { 13093562431584567480U, -502 },
    { 9755464219737475723U,  -475 },  { 14536774485912137811U, -449 },
    { 10830740992659433045U, -422 },  { 16139061738043178685U, -396 },
    { 12024538023802026127U, -369 },  { 17917957937422433684U, -343 },
    { 13349918974505688015U, -316 },  { 9946464728195732843U,  -289 },
    { 14821387422376473014U, -263 },  { 11042794154864902060U, -236 },
    { 16455045573212060422U, -210 },  { 12259964326927110867U, -183 },
    { 18268770466636286478U, -157 },  { 13611294676837538539U, -130 },
    { 10141204801825835212U, -103 },  { 15111572745182864684U, -77 },
    { 11258999068426240000U, -50 },   { 16777216000000000000U, -24 },
    { 12500000000000000000U,   3 },   { 9313225746154785156U,   30 },
    { 13877787807814456755U,  56 },   { 10339757656912845936U,  83 },
    { 15407439555097886824U, 109 },   { 11479437019748901445U, 136 },
    { 17105694144590052135U, 162 },   { 12744735289059618216U, 189 },
    { 9495567745759798747U,  216 },   { 14149498560666738074U, 242 },
    { 10542197943230523224U, 269 },   { 15709099088952724970U, 295 },
    { 11704190886730495818U, 322 },   { 17440603504673385349U, 348 },
    { 12994262207056124023U, 375 },   { 9681479787123295682U,  402 },
    { 14426529090290212157U, 428 },   { 10748601772107342003U, 455 },
    { 16016664761464807395U, 481 },   { 11933345169920330789U, 508 },
    { 17782069995880619868U, 534 },   { 13248674568444952270U, 561 },
    { 9871031767461413346U,  588 },   { 14708983551653345445U, 614 },
    { 10959046745042015199U, 641 },   { 16330252207878254650U, 667 },
    { 12166986024289022870U, 694 },   { 18130221999122236476U, 720 },
    { 13508068024458167312U, 747 },   { 10064294952495520794U, 774 },
    { 14996968138956309548U, 800 },   { 11173611982879273257U, 827 },
    { 16649979327439178909U, 853 },   { 12405201291620119593U, 880 },
    { 9242595204427927429U,  907 },   { 13772540099066387757U, 933 },
    { 10261342003245940623U, 960 },   { 15290591125556738113U, 986 },
    { 11392378155556871081U, 1013 },  { 16975966327722178521U, 1039 },
    { 12648080533535911531U, 1066 }
};

static __test_fp__ __test_find_cachedpow10__(int exp, int* k)
{
    const double one_log_ten = 0.30102999566398114;

    int approx = -(exp + __test_npowers__) * one_log_ten;
    int idx = (approx - __test_firstpower__) / __test_steppowers__;

    while(1) {
        int current = exp + __test_powers_ten__[idx].exp + 64;

        if(current < __test_expmin__) {
            idx++;
            continue;
        }

        if(current > __test_expmax__) {
            idx--;
            continue;
        }

        *k = (__test_firstpower__ + idx * __test_steppowers__);

        return __test_powers_ten__[idx];
    }
}

// from file fpconv.c

#define __test_fracmask__  0x000FFFFFFFFFFFFFU
#define __test_expmask__   0x7FF0000000000000U
#define __test_hiddenbit__ 0x0010000000000000U
#define __test_signmask__  0x8000000000000000U
#define __test_expbias__   (1023 + 52)

#define __test_abs__(n) ((n) < 0 ? -(n) : (n))
#define __test_min__(a, b) ((a) < (b) ? (a) : (b))

static uint64_t __test_tens__[] = {
    10000000000000000000U, 1000000000000000000U, 100000000000000000U,
    10000000000000000U, 1000000000000000U, 100000000000000U,
    10000000000000U, 1000000000000U, 100000000000U,
    10000000000U, 1000000000U, 100000000U,
    10000000U, 1000000U, 100000U,
    10000U, 1000U, 100U,
    10U, 1U
};

static inline uint64_t __test_get_dbits__(double d)
{
    union {
        double   dbl;
        uint64_t i;
    } dbl_bits = { d };

    return dbl_bits.i;
}

static __test_fp__ __test_build_fp__(double d)
{
    uint64_t bits = __test_get_dbits__(d);

    __test_fp__ fp;
    fp.frac = bits & __test_fracmask__;
    fp.exp = (bits & __test_expmask__) >> 52;

    if(fp.exp) {
        fp.frac += __test_hiddenbit__;
        fp.exp -= __test_expbias__;

    } else {
        fp.exp = -__test_expbias__ + 1;
    }

    return fp;
}

static void __test_normalize__(__test_fp__* fp)
{
    while ((fp->frac & __test_hiddenbit__) == 0) {
        fp->frac <<= 1;
        fp->exp--;
    }

    int shift = 64 - 52 - 1;
    fp->frac <<= shift;
    fp->exp -= shift;
}

static void __test_get_normalized_boundaries__(__test_fp__* fp, __test_fp__* lower, __test_fp__* upper)
{
    upper->frac = (fp->frac << 1) + 1;
    upper->exp  = fp->exp - 1;

    while ((upper->frac & (__test_hiddenbit__ << 1)) == 0) {
        upper->frac <<= 1;
        upper->exp--;
    }

    int u_shift = 64 - 52 - 2;

    upper->frac <<= u_shift;
    upper->exp = upper->exp - u_shift;


    int l_shift = fp->frac == __test_hiddenbit__ ? 2 : 1;

    lower->frac = (fp->frac << l_shift) - 1;
    lower->exp = fp->exp - l_shift;


    lower->frac <<= lower->exp - upper->exp;
    lower->exp = upper->exp;
}

static __test_fp__ __test_multiply__(__test_fp__* a, __test_fp__* b)
{
    const uint64_t lomask = 0x00000000FFFFFFFF;

    uint64_t ah_bl = (a->frac >> 32)    * (b->frac & lomask);
    uint64_t al_bh = (a->frac & lomask) * (b->frac >> 32);
    uint64_t al_bl = (a->frac & lomask) * (b->frac & lomask);
    uint64_t ah_bh = (a->frac >> 32)    * (b->frac >> 32);

    uint64_t tmp = (ah_bl & lomask) + (al_bh & lomask) + (al_bl >> 32);
    /* round up */
    tmp += 1U << 31;

    __test_fp__ fp = {
        ah_bh + (ah_bl >> 32) + (al_bh >> 32) + (tmp >> 32),
        a->exp + b->exp + 64
    };

    return fp;
}

static void __test_round_digit__(char* digits, int ndigits, uint64_t delta, uint64_t rem, uint64_t kappa, uint64_t frac)
{
    while (rem < frac && delta - rem >= kappa &&
           (rem + kappa < frac || frac - rem > rem + kappa - frac)) {

        digits[ndigits - 1]--;
        rem += kappa;
    }
}

static int __test_generate_digits__(__test_fp__* fp, __test_fp__* upper, __test_fp__* lower, char* digits, int* K)
{
    uint64_t wfrac = upper->frac - fp->frac;
    uint64_t delta = upper->frac - lower->frac;

    __test_fp__ one;
    one.frac = 1ULL << -upper->exp;
    one.exp  = upper->exp;

    uint64_t part1 = upper->frac >> -one.exp;
    uint64_t part2 = upper->frac & (one.frac - 1);

    int idx = 0, kappa = 10;
    uint64_t* divp;
    /* 1000000000 */
    for(divp = __test_tens__ + 10; kappa > 0; divp++) {

        uint64_t div = *divp;
        unsigned digit = part1 / div;

        if (digit || idx) {
            digits[idx++] = digit + '0';
        }

        part1 -= digit * div;
        kappa--;

        uint64_t tmp = (part1 <<-one.exp) + part2;
        if (tmp <= delta) {
            *K += kappa;
            __test_round_digit__(digits, idx, delta, tmp, div << -one.exp, wfrac);

            return idx;
        }
    }

    /* 10 */
    uint64_t* unit = __test_tens__ + 18;

    while(1) {
        part2 *= 10;
        delta *= 10;
        kappa--;

        unsigned digit = part2 >> -one.exp;
        if (digit || idx) {
            digits[idx++] = digit + '0';
        }

        part2 &= one.frac - 1;
        if (part2 < delta) {
            *K += kappa;
            __test_round_digit__(digits, idx, delta, part2, one.frac, wfrac * *unit);

            return idx;
        }

        unit--;
    }
}

static int __test_grisu2__(double d, char* digits, int* K)
{
    __test_fp__ w = __test_build_fp__(d);

    __test_fp__ lower, upper;
    __test_get_normalized_boundaries__(&w, &lower, &upper);

    __test_normalize__(&w);

    int k;
    __test_fp__ cp = __test_find_cachedpow10__(upper.exp, &k);

    w     = __test_multiply__(&w,     &cp);
    upper = __test_multiply__(&upper, &cp);
    lower = __test_multiply__(&lower, &cp);

    lower.frac++;
    upper.frac--;

    *K = -k;

    return __test_generate_digits__(&w, &upper, &lower, digits, K);
}

static int __test_emit_digits__(char* digits, int ndigits, char* dest, int K, int neg)
{
    int exp = __test_abs__(K + ndigits - 1);

    int max_trailing_zeros = 7;

    if(neg) {
        max_trailing_zeros -= 1;
    }

    /* write plain integer */
    if(K >= 0 && (exp < (ndigits + max_trailing_zeros))) {

        memcpy(dest, digits, ndigits);
        memset(dest + ndigits, '0', K);

        return ndigits + K;
    }

    /* write decimal w/o scientific notation */
    if(K < 0 && (K > -7 || exp < 4)) {
        int offset = ndigits - __test_abs__(K);
        /* fp < 1.0 -> write leading zero */
        if(offset <= 0) {
            offset = -offset;
            dest[0] = '0';
            dest[1] = '.';
            memset(dest + 2, '0', offset);
            memcpy(dest + offset + 2, digits, ndigits);

            return ndigits + 2 + offset;

        /* fp > 1.0 */
        } else {
            memcpy(dest, digits, offset);
            dest[offset] = '.';
            memcpy(dest + offset + 1, digits + offset, ndigits - offset);

            return ndigits + 1;
        }
    }

    /* write decimal w/ scientific notation */
    ndigits = __test_min__(ndigits, 18 - neg);

    int idx = 0;
    dest[idx++] = digits[0];

    if(ndigits > 1) {
        dest[idx++] = '.';
        memcpy(dest + idx, digits + 1, ndigits - 1);
        idx += ndigits - 1;
    }

    dest[idx++] = 'e';

    char sign = K + ndigits - 1 < 0 ? '-' : '+';
    dest[idx++] = sign;

    int cent = 0;

    if(exp > 99) {
        cent = exp / 100;
        dest[idx++] = cent + '0';
        exp -= cent * 100;
    }
    if(exp > 9) {
        int dec = exp / 10;
        dest[idx++] = dec + '0';
        exp -= dec * 10;

    } else if(cent) {
        dest[idx++] = '0';
    }

    dest[idx++] = exp % 10 + '0';

    return idx;
}

static int __test_filter_special__(double fp, char* dest)
{
    if(fp == 0.0) {
        dest[0] = '0';
        return 1;
    }

    uint64_t bits = __test_get_dbits__(fp);

    int nan = (bits & __test_expmask__) == __test_expmask__;

    if(!nan) {
        return 0;
    }

    if(bits & __test_fracmask__) {
        dest[0] = 'n'; dest[1] = 'a'; dest[2] = 'n';

    } else {
        dest[0] = 'i'; dest[1] = 'n'; dest[2] = 'f';
    }

    return 3;
}

static int __test_fpconv_dtoa__(double d, char dest[24])
{
    char digits[18];

    int str_len = 0;
    int neg = 0;

    if(__test_get_dbits__(d) & __test_signmask__) {
        dest[0] = '-';
        str_len++;
        neg = 1;
    }

    int spec = __test_filter_special__(d, dest + str_len);

    if(spec) {
        return str_len + spec;
    }

    int K = 0;
    int ndigits = __test_grisu2__(d, digits, &K);

    str_len += __test_emit_digits__(digits, ndigits, dest + str_len, K, neg);

    return str_len;
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // #ifndef UNIT_NO_FP

// MARK: - buffer

typedef struct __test_buffer__ {
    char* memory;
    size_t size;
    size_t reserved;
} __test_buffer__;

#define __TEST_BUFFER_MIN_SIZE 512

static void __test_buffer_reserve__(__test_buffer__* buffer, size_t size) {
    if (buffer->memory == 0) {
        size_t new_capacity = __TEST_MAX(size, __TEST_BUFFER_MIN_SIZE);
        char* new_memory = (char*)UNIT_ALLOC(new_capacity);
        if (!new_memory) {
            UNIT_ABORT();
        }
        buffer->memory = new_memory;
        buffer->reserved = new_capacity;
    }
    else if (buffer->size + size > buffer->reserved) {
        size_t new_capacity = __TEST_MAX(buffer->size + size, buffer->size * 2);
        char* new_memory = (char*)UNIT_ALLOC(new_capacity);
        if (!new_memory) {
            UNIT_ABORT();
        }
        UNIT_MEMCPY(new_memory, buffer->memory, buffer->size);
        UNIT_FREE(buffer->memory);
        buffer->memory = new_memory;
        buffer->reserved = new_capacity;
    }
}

static void __test_buffer_push__(__test_buffer__* buffer, const char* data, size_t size) {
    __test_buffer_reserve__(buffer, size);
    UNIT_MEMCPY(buffer->memory + buffer->size, data, size);
    buffer->size += size;
};

// helper macros to use __test_buffer__ in a typed way (tbuffer)

#define __test_tbuffer_size__(type, bufferptr) ((bufferptr)->size/sizeof(type))

#define __test_tbuffer_push__(type, bufferptr, element)                         \
    do {                                                                        \
        type __test_element_tmp__ = (element);                                  \
        __test_buffer_push__(bufferptr, &__test_element_tmp__, sizeof(type));   \
    } while(0)

#define __test_tbuffer_ppush__(type, bufferptr, elementptr)                     \
    do {                                                                        \
        __test_buffer_push__(bufferptr, (const char*)(elementptr), sizeof(type));\
    } while(0)

#define __test_tbuffer_pop__(type, bufferptr)                                   \
    do{                                                                         \
        (bufferptr)->size -= sizeof(type);                                      \
    } while(0)

#define __test_tbuffer_elements__(type, bufferptr)                              \
    ((type*) ((bufferptr)->memory))


#define __test_tbuffer_at__(type, bufferptr, idx)                               \
    (((type*) ((bufferptr)->memory)) + (idx))

#define __test_tbuffer_top__(type, bufferptr)                                   \
    (                                                                           \
        __test_tbuffer_elements__(type, bufferptr) +                            \
        __test_tbuffer_size__(type, bufferptr) - 1                              \
    )

// macros and functions to add formatted text to the buffer
#define __test_buffer_push_str__(bufferptr, s) __test_buffer_push__((bufferptr), (s), UNIT_STRLEN(s))

static size_t __test_back_push_uint_unsafe__(char* buffer, uint64_t v) {
    static const char NUM_MAP[] =
        "00" "01" "02" "03" "04" "05" "06" "07" "08" "09"
        "10" "11" "12" "13" "14" "15" "16" "17" "18" "19"
        "20" "21" "22" "23" "24" "25" "26" "27" "28" "29"
        "30" "31" "32" "33" "34" "35" "36" "37" "38" "39"
        "40" "41" "42" "43" "44" "45" "46" "47" "48" "49"
        "50" "51" "52" "53" "54" "55" "56" "57" "58" "59"
        "60" "61" "62" "63" "64" "65" "66" "67" "68" "69"
        "70" "71" "72" "73" "74" "75" "76" "77" "78" "79"
        "80" "81" "82" "83" "84" "85" "86" "87" "88" "89"
        "90" "91" "92" "93" "94" "95" "96" "97" "98" "99";
    size_t result = 0;
    while (v) {
        uint64_t n = v%100;
        v = v/100;
        buffer-=2;
        *(uint16_t*)buffer = *(uint16_t*)&NUM_MAP[n*2];
        result += 2;
    }
    if (result == 0) {
        *(buffer - 1) = '0';
        return 1;
    }
    if (*buffer == '0') {
        buffer+=1;
        result-=1;
    }
    return result;
}

static void __test_buffer_push_fmt_uint__(__test_buffer__* buffer, uint64_t v) {
    char TMP[20];
    size_t sz = __test_back_push_uint_unsafe__(TMP + 20, v);
    __test_buffer_push__(buffer, TMP + 20 -sz, sz);
}

static void __test_buffer_push_fmt_int__(__test_buffer__* buffer, int64_t v) {
    char TMP[21];
    if (v >= 0) {
        size_t sz = __test_back_push_uint_unsafe__(TMP + 21, v);
        __test_buffer_push__(buffer, TMP + 21 -sz, sz);
    }
    else {
        size_t sz = __test_back_push_uint_unsafe__(TMP + 21, -v);
        *(TMP + 21 -sz - 1) = '-';
        __test_buffer_push__(buffer, TMP + 21 -sz - 1, sz + 1);
    }
}

static void __test_buffer_push_fmt_double__(__test_buffer__* buffer, double v) {
    char TMP[24];
    int res = __test_fpconv_dtoa__(v, TMP);
    __test_buffer_push__(buffer, TMP, (size_t)res);
}

// MARK: - test registry

typedef struct __test_registry__ {
    const char* name;
    const char* tags;
    __test_case_fn__ fn;
} __test_registry__;


static __test_buffer__ __test_registered_tests__ = {0, 0, 0};

int __test_register_test__(__test_case_fn__ fn, const char* fn_name, const char* tags) {
    __test_registry__ registry;
    registry.name = fn_name;
    registry.tags = tags;
    registry.fn = fn;
    __test_tbuffer_ppush__(__test_registry__, &__test_registered_tests__, &registry);
    return 0;
}

static const __test_registry__* __test_get_test_registry__(size_t* elements) {
    *elements =  __test_tbuffer_size__(__test_registry__, &__test_registered_tests__);
    return __test_tbuffer_elements__(__test_registry__, &__test_registered_tests__);
}

// MARK: - path execution

#define __test_flow_flag_has_next__ 1
#define __test_flow_flag_is_section__ 2

typedef struct __test_flow_path__ {
    size_t index;
    unsigned flags;
    const char* description;
} __test_flow_path__;

typedef struct __test_section_tracking__ {
    size_t flow_start;
    size_t current_section_number;
} __test_section_tracking__;

static size_t __test_flow_index__;
static int __test_scope_has_run_section__;
static __test_buffer__  __test_flow_buffer__;
static __test_buffer__  __test_section_stack__;

size_t __test_generate__(const char* name, size_t n) {
    size_t max_flow_index = __test_tbuffer_size__(__test_flow_path__, &__test_flow_buffer__);
    size_t current_idx = __test_flow_index__++;
    if (max_flow_index <= current_idx) {
        __test_flow_path__ path;
        path.description = name;
        path.index = 0;
        path.flags = 0;
        if (n > 0) {
            path.flags |= __test_flow_flag_has_next__;
        }
        __test_tbuffer_ppush__(__test_flow_path__, &__test_flow_buffer__, &path);
        return 0;
    }
    else {
        __test_flow_path__* path = __test_tbuffer_at__(__test_flow_path__, &__test_flow_buffer__, current_idx);
        path->description = name;
        path->flags = 0;
        if (n != path->index + 1) {
            path->flags |= __test_flow_flag_has_next__;
        }
        return path->index;
    }
}

int __test_section_start__(const char* name) {
    __test_section_tracking__* tracking;
    __test_flow_path__* path;

    if (__test_scope_has_run_section__) {
        tracking = __test_tbuffer_top__(__test_section_tracking__, &__test_section_stack__);
        path = __test_tbuffer_at__(__test_flow_path__, &__test_flow_buffer__, tracking->flow_start);
    }
    else {
        __test_section_tracking__ t;
        t.flow_start = __test_flow_index__;
        t.current_section_number = 0;
        __test_tbuffer_ppush__(__test_section_tracking__, &__test_section_stack__, &t);
        tracking = __test_tbuffer_top__(__test_section_tracking__, &__test_section_stack__);

        size_t max_flow_index = __test_tbuffer_size__(__test_flow_path__, &__test_flow_buffer__);
        size_t current_idx = __test_flow_index__++;
        if (max_flow_index <= current_idx) {
            __test_flow_path__ p;
            p.description = name;
            p.index = 0;
            p.flags = __test_flow_flag_is_section__;
            __test_tbuffer_ppush__(__test_flow_path__, &__test_flow_buffer__, &p);
        }
        path = __test_tbuffer_at__(__test_flow_path__, &__test_flow_buffer__, current_idx);
    }

    if (tracking->current_section_number == path->index) {
        // we need to run the section
        __test_scope_has_run_section__ = 0;
        return 0;
    }
    else {
        // skip
        if (tracking->current_section_number > path->index) {
            path->flags |= __test_flow_flag_has_next__;
        }
        tracking->current_section_number++;
        __test_scope_has_run_section__ = 1;
        return 1;
    }
}

int __test_section_end__() {
    if (__test_scope_has_run_section__) {
        __test_tbuffer_pop__(__test_section_tracking__, &__test_section_stack__);
    }
    __test_section_tracking__* tracking = __test_tbuffer_top__(__test_section_tracking__, &__test_section_stack__);
    tracking->current_section_number++;
    __test_scope_has_run_section__ = 1;
    return 1;
}

// ---- scoped alloc and scoped defer ----

typedef struct __test_alloc_data__ {
    void* memory;
} __test_alloc_data__;

typedef struct __test_defer_data__ {
    void* data;
    __test_defer_fn__ fn;
} __test_defer_data__;

// static __test_mutex__ __test_alloc_mutex__;
static __test_buffer__ __test_alloc_stack__;

// static __test_mutex__ __test_defer_mutex__;
static __test_buffer__ __test_defer_stack__;


void* __test_scoped_alloc__(size_t size) {
    void* result = UNIT_ALLOC(size);
    if (result == 0) {
        return result;
    }
    __test_alloc_data__ data;
    data.memory = result;
    // __test_mutex_lock__(&__test_alloc_mutex__);
    __test_tbuffer_ppush__(__test_alloc_data__, &__test_alloc_stack__, &data);
    // __test_mutex_unlock__(&__test_alloc_mutex__);
    return result;
}

void __test_defer__(void* ptr, __test_defer_fn__ fn)  {
    __test_defer_data__ data;
    data.data = ptr;
    data.fn = fn;
    // __test_mutex_lock__(&__test_defer_mutex__);
    __test_tbuffer_ppush__(__test_defer_data__, &__test_defer_stack__, &data);
    // __test_mutex_unlock__(&__test_defer_mutex__);
}

// MARK: - format assertions

static int __test_has_error_flag__ = 0;

int __test_has_error__() {
    return __test_has_error_flag__;
}

struct __test_log_buffer__ {
    const char* fmt_ptr;
    int state; // 0 -> complete, 1 -> waiting for input
    size_t line;
    const char* filename;
    __test_buffer__ buff;
};

static void __test_log_buffer_parse_format__(__test_log_buffer__* fmt) {
    const char* start = fmt->fmt_ptr;
    const char* it = fmt->fmt_ptr;
    while (*it != 0) {
        if (*it == '{') {
            if (*(it + 1) == '}') {
                __test_buffer_push__(&fmt->buff, start, it - start);
                fmt->fmt_ptr = it + 2;
                fmt->state = 1;
                return;
            }
            if (*(it + 1) == '{') {
                __test_buffer_push__(&fmt->buff, start, it + 1 - start);
                start = it + 2;
                it += 1;
            }
        }
        else if (*it == '}' && *(it + 1) == '}') {
            __test_buffer_push__(&fmt->buff, start, it + 1 - start);
            start = it + 2;
            it += 1;
        }
        it++;
    }
    __test_buffer_push__(&fmt->buff, start, it - start);
    fmt->fmt_ptr = it;
    fmt->state = 0;
}

__test_log_buffer__* __test_log_buffer_create__(size_t line, const char* filename, const char* fmt) {
    __test_log_buffer__* result = (__test_log_buffer__*)UNIT_ALLOC(sizeof(__test_log_buffer__));
    memset(result, 0, sizeof(__test_log_buffer__));
    result->line = line;
    result->filename = filename;
    result->fmt_ptr = fmt;
    __test_log_buffer_parse_format__(result);
    return result;
}

void __test_log_buffer_log__(__test_log_buffer__* buff, const char* macro_name, int is_error) {
    if (buff->state != 0) {
        UNIT_ABORT();
    }
    char TMP[23];
    TMP[21] = ']';
    TMP[22] = ' ';
    size_t sz = __test_back_push_uint_unsafe__(TMP + 21, buff->line);
    TMP[21 - sz - 1] = ':';
    UNIT_WRITEOUTPUT(0, macro_name, UNIT_STRLEN(macro_name));
    UNIT_WRITEOUTPUT(0, buff->filename, UNIT_STRLEN(buff->filename));
    UNIT_WRITEOUTPUT(0, TMP + (21 - sz - 1), sz + 3);
    UNIT_WRITEOUTPUT(1, buff->buff.memory, buff->buff.size);
    UNIT_FREE(buff);
    if (is_error) {
        __test_has_error_flag__ = 1;
    }
}

void __test_log_buffer_add_str__(__test_log_buffer__* buff, const char* str, size_t sz) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    __test_buffer_push__(&buff->buff, str, sz);
    __test_log_buffer_parse_format__(buff);
}

void __test_log_buffer_add_fmt_int__(__test_log_buffer__* buff, int64_t v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    __test_buffer_push_fmt_int__(&buff->buff, v);
    __test_log_buffer_parse_format__(buff);
}

void __test_log_buffer_add_fmt_uint__(__test_log_buffer__* buff, uint64_t v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    __test_buffer_push_fmt_uint__(&buff->buff, v);
    __test_log_buffer_parse_format__(buff);
}

void __test_log_buffer_add_fmt_double__(__test_log_buffer__* buff, double v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    __test_buffer_push_fmt_double__(&buff->buff, v);
    __test_log_buffer_parse_format__(buff);
}

// MARK: - runner

static int __test_next_flow__() {
    size_t size = __test_tbuffer_size__(__test_flow_path__, &__test_flow_buffer__);
    __test_flow_path__* paths = __test_tbuffer_elements__(__test_flow_path__, &__test_flow_buffer__);
    for (size_t i = size - 1; i < size; i--) {
        __test_flow_path__* p = paths + i;
        if (p->flags & __test_flow_flag_has_next__) {
            p->index += 1;
            p->flags = 0;
            __test_flow_buffer__.size = (i + 1) * sizeof(__test_flow_path__);
            return 1;
        }
    }
    return 0;
}

void __test_log_current_flow__() {
    size_t size = __test_tbuffer_size__(__test_flow_path__, &__test_flow_buffer__);
    __test_flow_path__* paths = __test_tbuffer_elements__(__test_flow_path__, &__test_flow_buffer__);
    for (size_t i = 0; i < size; i++) {
        __test_flow_path__* p = paths + i;
        if (p->flags & __test_flow_flag_is_section__) {
            UNIT_WRITEOUTPUT(0, "    CASE: ", 10);
        }
        else {
            UNIT_WRITEOUTPUT(0, "    GENERATE ", 13);
            char TMP[21];
            TMP[20] = ':';
            size_t sz = __test_back_push_uint_unsafe__(TMP + 20, p->index);
            UNIT_WRITEOUTPUT(0, TMP + 20 - sz, sz + 1);
        }
        UNIT_WRITEOUTPUT(1, p->description, UNIT_STRLEN(p->description));
    }
}

static int __test_run_test__(const __test_registry__* reg) {
    int r = 0;
    UNIT_WRITEOUTPUT(1, "----------------------------------------", 40);
    UNIT_WRITEOUTPUT(0, "Running test \"", 14);
    UNIT_WRITEOUTPUT(0, reg->name, strlen(reg->name));
    UNIT_WRITEOUTPUT(1, "\"", 1);
    UNIT_WRITEOUTPUT(1, "", 0);
    size_t num_cases = 0;
    size_t num_failed_cases = 0;

    __test_flow_buffer__.size = 0;
    do {
        __test_section_stack__.size = 0;
        __test_scope_has_run_section__ = 0;
        __test_flow_index__ = 0;
        __test_has_error_flag__ = 0;
        int result = UNIT_RUNEXCEPT(reg->fn);
        {
            // Run all the defered
            // TODO maybe we also need to catch exceptions here?
            size_t size = __test_tbuffer_size__(__test_defer_data__, &__test_defer_stack__);
            if (size) {
                __test_defer_data__* first = __test_tbuffer_elements__(__test_defer_data__, &__test_defer_stack__);
                __test_defer_data__* it = first + size -1;
                for (; it >= first; it--) {
                    it->fn(it->data);
                }
                __test_defer_stack__.size = 0;
            }
        }
        {
            // Run all scopped free
            size_t size = __test_tbuffer_size__(__test_alloc_data__, &__test_alloc_stack__);
            if (size) {
                __test_alloc_data__* first = __test_tbuffer_elements__(__test_alloc_data__, &__test_alloc_stack__);
                __test_alloc_data__* it = first + size -1;
                for (; it >= first; it--) {
                    UNIT_FREE(it->memory);
                }
                __test_alloc_stack__.size = 0;
            }
        }
        result |= __test_has_error__();
        if (result) {
            r = 1;
            UNIT_WRITEOUTPUT(1, "Failed case", 11);
            __test_log_current_flow__();
            num_failed_cases++;
        }
        num_cases++;
    } while(__test_next_flow__());

    char TMP[20];
    UNIT_WRITEOUTPUT(1, "", 0);
    UNIT_WRITEOUTPUT(1, "Test results:", 13);
    UNIT_WRITEOUTPUT(0, "    Failed cases: ", 18);
    size_t num_size = __test_back_push_uint_unsafe__(TMP + 20, num_failed_cases);
    UNIT_WRITEOUTPUT(1, TMP + 20 - num_size, num_size);
    UNIT_WRITEOUTPUT(0, "    Total cases: ", 17);
    num_size = __test_back_push_uint_unsafe__(TMP + 20, num_cases);
    UNIT_WRITEOUTPUT(1, TMP + 20 - num_size, num_size);
    UNIT_WRITEOUTPUT(1, "", 0);
    if (r) {
        UNIT_WRITEOUTPUT(1, "FAILED", 6);
    }
    else {
        UNIT_WRITEOUTPUT(1, "SUCCEEDED", 9);
    }
    return r;
}

#ifdef __TEST_REQUIRE_STATIC_INIT__
    static void __test_static_init__();
    #error "TODO: provide a C99 standard compliant way to register tests"
#endif

int main(int argc, const char* argv[]) {
#ifdef __TEST_REQUIRE_STATIC_INIT__
    __test_static_init__();
#endif
    size_t num_tests;
    const __test_registry__* tests = __test_get_test_registry__(&num_tests);
    for (size_t i = 0; i < num_tests; i++) {
        __test_run_test__(tests + i);
    }
}

#endif


#ifdef _MSC_VER
#pragma warning( pop )
#endif
