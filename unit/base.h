#ifndef UNIT_NO_STDLIB_H
#include <stdlib.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 5045)//, justification : "Spectre mitigation")
#endif

// redefine ofuscated typenames
typedef UNIT_NAME(long_int) unit_long_int_t;
typedef UNIT_NAME(long_uint) unit_long_uint_t;
typedef UNIT_NAME(unit_size) unit_size_t;
typedef UNIT_NAME(case_fn) unit_case_fn_t;
typedef UNIT_NAME(defer_fn) unit_defer_fn_t;
typedef UNIT_NAME(log_buffer) unit_log_buffer_t;

// check size of the types
static char __unit_check_long_int_t_size[8 == sizeof(unit_long_int_t) ? 1 : -1];
static char __unit_check_long_uint_t_size[8 == sizeof(unit_long_uint_t) ? 1 : -1];
static char __unit_check_size_t_size[sizeof(void*) == sizeof(unit_size_t) ? 1 : -1];

// forward declare important internal types
typedef struct unit_test_registry_t unit_test_registry_t;

#ifndef UNIT_MAX
#define UNIT_MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef UNIT_ALLOC
#define UNIT_ALLOC(x) malloc(x)
#endif

#ifndef UNIT_FREE
#define UNIT_FREE(x) free(x)
#endif

#ifndef UNIT_ABORT
#define UNIT_ABORT() abort()
#endif

#ifndef UNIT_STRLEN
static unit_size_t _unit_strlen(const char* str) {
    unit_size_t s = 0;
    while(*(str++)) {
        s++;
    }
    return s;
}
#define UNIT_STRLEN(str) _unit_strlen(str)
#endif

#ifndef UNIT_MEMCPY
static void _unit_memcpy(void* dest, const void* src, unit_size_t size) {
    char* dest_char = (char*)dest;
    const char* src_char = (char*)src;
    const char* src_end = src_char + size;
    for(; src_char < src_end; src_char++) {
        *(dest_char++) = *src_char;
    }
}
#define UNIT_MEMCPY(des, src, size) _unit_memcpy(des, src, size)
#endif

#ifndef UNIT_MEMSET
static void _unit_memset(void* dest, int val, unit_size_t size) {
    char* dest_char = (char*)dest;
    char* dest_end = dest_char+size;
    for(; dest_char < dest_end; dest_char++) {
        *dest_char = (char)val;
    }
}
#define UNIT_MEMSET(des, val, size) _unit_memset(des, val, size)
#endif

#ifndef UNIT_MEMCMP
static int _unit_memcmp(const void* buff1, const void* buff2, unit_size_t size) {
    const char* buff1_char = (char*)buff1;
    const char* buff2_char = (char*)buff2;
    const char* buff1_end = buff1_char + size;
    for(; buff1_char < buff1_end; buff1_char++) {
        char c1 = *buff1_char;
        char c2 = *(buff2_char)++;
        if (c1 < c2) {
            return -1;
        }
        if (c1 > c2) {
            return 1;
        }
    }
    return 0;
}
#define UNIT_MEMCMP(des, src, size) _unit_memcmp(des, src, size)
#endif

#ifndef UNIT_IGNORE_UNUSED
#define UNIT_IGNORE_UNUSED(x) (void*)&x;
#endif

// strlen implementation
unit_size_t UNIT_NAME(unit_strlen)(const char* str) {
    return (unit_size_t)UNIT_STRLEN(str);
}
