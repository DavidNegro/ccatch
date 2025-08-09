#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable : 4820)
#pragma warning(disable : 5045)
#endif

// declarations from base.i/public.i required to include
// buffer.i
typedef size_t unit_size_t;
typedef unsigned long long unit_long_uint_t;
typedef long long unit_long_int_t;

#define UNIT_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UNIT_MEMCPY(des, src, len) memcpy(des, src, len)
#define UNIT_STRLEN(x) strlen(x)

#define UNIT_ABORT() abort()
#define UNIT_ALLOC(x) malloc(x)
#define UNIT_FREE(x) free(x)

int unit_fpconv_dtoa(double v, char TMP[24]) {
    // ignore unused
    (void)(v);
    (void)(TMP);
    return 0;
}

#include "buffer.h"

void buffer_push_file(unit_buffer_t* buffer, const char* path) {
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    unit_buffer_reserve(buffer, fsize);
    fread(buffer->memory + buffer->size, 1, fsize, f);
    buffer->size += fsize;
    fclose(f);
}

void buffer_write_file(const unit_buffer_t* buffer, const char* path) {
    FILE* f = fopen(path, "wb");
    fwrite(buffer->memory, 1, buffer->size, f);
    fclose(f);
}

typedef struct template_line_t {
    int is_include;
    const char* line;
} template_line_t;

#define LINE(x) {0, x "\n"},
#define INCLUDE(x) {1, x},

template_line_t template_header[] = {
    LINE("#ifndef __UNIT_INCLUDE_H__")
    LINE("#define __UNIT_INCLUDE_H__")
    INCLUDE("unit/public.h")
    LINE("#endif // #ifndef __UNIT_INCLUDE_H__")
    LINE("#ifdef UNIT_IMPL")
    LINE("#ifndef UNIT_NO_TEST // skip implementation if NO_TEST option is set")
    INCLUDE("unit/base.h")
    INCLUDE("unit/fpconv.h")
    INCLUDE("unit/mutex.h")
    INCLUDE("unit/buffer.h")
    INCLUDE("unit/runexcept.h")
    INCLUDE("unit/log_buffer.h")
    INCLUDE("unit/defer_alloc.h")
    INCLUDE("unit/subcase_tracker.h")
    INCLUDE("unit/platform_declarations.h")
    INCLUDE("unit/test_registry.h")
    INCLUDE("unit/test_specs.h")
    INCLUDE("unit/test_runner.h")
    LINE("#endif // #ifndef UNIT_NO_TEST")
    LINE("#endif // #ifdef UNIT_IMPL")
    {0, 0},
};

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        return -1;
    }

    unit_buffer_t output_buffer = {0, 0, 0};

    template_line_t* ln = template_header;
    while (ln->line) {
        if (ln->is_include) {
            buffer_push_file(&output_buffer, ln->line);
        }
        else {
            unit_buffer_push_str(&output_buffer, ln->line);
        }
        ln++;
    }

    buffer_write_file(&output_buffer, argv[1]);
    return 0;
}
