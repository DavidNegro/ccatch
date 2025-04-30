#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define UNIT_NAME(x) x
#define UNIT_STRLEN(x) strlen(x)
#include "base.i"
#include "buffer.i"

void buffer_push_file(buffer* buffer, const char* path) {
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer_reserve(buffer, fsize);
    fread(buffer->memory + buffer->size, 1, fsize, f);
    buffer->size += fsize;
    fclose(f);
}

void buffer_write_file(const buffer* buffer, const char* path) {
    FILE* f = fopen(path, "wb");
    fwrite(buffer->memory, 1, buffer->size, f);
    fclose(f);
}

typedef struct template_line {
    int is_include;
    const char* line;
} template_line;

#define LINE(x) {0, x "\n"},
#define INCLUDE(x) {1, x},

template_line template_header[] = {
    LINE("#ifndef __UNIT_INCLUDE_H__")
    LINE("#define __UNIT_INCLUDE_H__")
    INCLUDE("unit/public.i")
    LINE("#endif // #ifndef __UNIT_INCLUDE_H__")
    LINE("#ifdef UNIT_IMPL")
    INCLUDE("unit/base.i")
    INCLUDE("unit/fpconv.i")
    INCLUDE("unit/mutex.i")
    INCLUDE("unit/buffer.i")
    INCLUDE("unit/runexcept.i")
    INCLUDE("unit/log_buffer.i")
    INCLUDE("unit/defer_alloc.i")
    INCLUDE("unit/subcase_tracker.i")
    INCLUDE("unit/platform_declarations.i")
    INCLUDE("unit/test_registry.i")
    INCLUDE("unit/test_runner.i")
    LINE("#endif // UNIT_IMPL")
    {0, 0},
};

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        return -1;
    }

    buffer output_buffer = {0, 0, 0};

    template_line* ln = template_header;
    while (ln->line) {
        if (ln->is_include) {
            buffer_push_file(&output_buffer, ln->line);
        }
        else {
            __test_buffer_push_str__(&output_buffer, ln->line);
        }
        ln++;
    }

    buffer_write_file(&output_buffer, argv[1]);
    return 0;
}
