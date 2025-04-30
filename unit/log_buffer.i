/* Log buffer
*/

struct UNIT_NAME(log_buffer) {
    const char* fmt_ptr;
    int state; // 0 -> complete, 1 -> waiting for input
    size_t line;
    const char* filename;
    UNIT_NAME(buffer) buff;
};

static void UNIT_NAME(log_buffer_parse_format)(UNIT_NAME(log_buffer)* fmt) {
    const char* start = fmt->fmt_ptr;
    const char* it = fmt->fmt_ptr;
    while (*it != 0) {
        if (*it == '{') {
            if (*(it + 1) == '}') {
                UNIT_NAME(buffer_push)(&fmt->buff, start, it - start);
                fmt->fmt_ptr = it + 2;
                fmt->state = 1;
                return;
            }
            if (*(it + 1) == '{') {
                UNIT_NAME(buffer_push)(&fmt->buff, start, it + 1 - start);
                start = it + 2;
                it += 1;
            }
        }
        else if (*it == '}' && *(it + 1) == '}') {
            UNIT_NAME(buffer_push)(&fmt->buff, start, it + 1 - start);
            start = it + 2;
            it += 1;
        }
        it++;
    }
    UNIT_NAME(buffer_push)(&fmt->buff, start, it - start);
    fmt->fmt_ptr = it;
    fmt->state = 0;
}

UNIT_NAME(log_buffer)* UNIT_NAME(log_buffer_create)(size_t line, const char* filename, const char* fmt) {
    UNIT_NAME(log_buffer)* result = (UNIT_NAME(log_buffer)*)UNIT_ALLOC(sizeof(__test_log_buffer__));
    UNIT_MEMSET(result, 0, sizeof(UNIT_NAME(log_buffer)));
    result->line = line;
    result->filename = filename;
    result->fmt_ptr = fmt;
    UNIT_NAME(log_buffer_parse_format)(result);
    return result;
}

static void UNIT_NAME(log_buffer_destroy)(UNIT_NAME(log_buffer)* buffer) {
    UNIT_NAME(buffer_deinit)(&buffer->buff);
    UNIT_FREE(buffer);
}

int UNIT_NAME(log_buffer_add_str)(UNIT_NAME(log_buffer)* buff, const char* str, size_t sz) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    UNIT_NAME(buffer_push)(&buff->buff, str, sz);
    UNIT_NAME(log_buffer_parse_format)(buff);
    return 0;
}

int UNIT_NAME(log_buffer_add_fmt_int)(UNIT_NAME(log_buffer)* buff, int64_t v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    UNIT_NAME(buffer_push_fmt_int)(&buff->buff, v);
    UNIT_NAME(log_buffer_parse_format)(buff);
    return 0;
}

int UNIT_NAME(log_buffer_add_fmt_uint)(UNIT_NAME(log_buffer)* buff, uint64_t v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    UNIT_NAME(buffer_push_fmt_uint)(&buff->buff, v);
    UNIT_NAME(log_buffer_parse_format)(buff);
    return 0;
}

int UNIT_NAME(log_buffer_add_fmt_double)(UNIT_NAME(log_buffer)* buff, double v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    UNIT_NAME(buffer_push_fmt_double)(&buff->buff, v);
    UNIT_NAME(log_buffer_parse_format)(buff);
    return 0;
}
