/* Log buffer
*/

struct UNIT_NAME(log_buffer) {
    const char* fmt_ptr;
    const char* filename;
    unit_buffer_t buff;
    unit_size_t line;
    unit_size_t state; // 0 -> complete, 1 -> waiting for input
};

static void _unit_log_buffer_parse_format(unit_log_buffer_t* fmt) {
    const char* start = fmt->fmt_ptr;
    const char* it = fmt->fmt_ptr;
    while (*it != 0) {
        if (*it == '{') {
            if (*(it + 1) == '}') {
                unit_buffer_push(&fmt->buff, start, it - start);
                fmt->fmt_ptr = it + 2;
                fmt->state = 1;
                return;
            }
            if (*(it + 1) == '{') {
                unit_buffer_push(&fmt->buff, start, it + 1 - start);
                start = it + 2;
                it += 1;
            }
        }
        else if (*it == '}' && *(it + 1) == '}') {
            unit_buffer_push(&fmt->buff, start, it + 1 - start);
            start = it + 2;
            it += 1;
        }
        it++;
    }
    unit_buffer_push(&fmt->buff, start, it - start);
    fmt->fmt_ptr = it;
    fmt->state = 0;
}

unit_log_buffer_t* UNIT_NAME(log_buffer_create)(unit_size_t line, const char* filename, const char* fmt) {
    unit_log_buffer_t* result = (unit_log_buffer_t*)UNIT_ALLOC(sizeof(unit_log_buffer_t));
    UNIT_MEMSET(result, 0, sizeof(unit_log_buffer_t));
    result->line = line;
    result->filename = filename;
    result->fmt_ptr = fmt;
    _unit_log_buffer_parse_format(result);
    return result;
}

static void unit_log_buffer_destroy(UNIT_NAME(log_buffer)* buffer) {
    unit_buffer_deinit(&buffer->buff);
    UNIT_FREE(buffer);
}

int UNIT_NAME(log_buffer_add_str)(UNIT_NAME(log_buffer)* buff, const char* str, unit_size_t sz) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    unit_buffer_push(&buff->buff, str, sz);
    _unit_log_buffer_parse_format(buff);
    return 0;
}

int UNIT_NAME(log_buffer_add_fmt_int)(UNIT_NAME(log_buffer)* buff, unit_long_int_t v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    unit_buffer_push_fmt_int(&buff->buff, v);
    _unit_log_buffer_parse_format(buff);
    return 0;
}

int UNIT_NAME(log_buffer_add_fmt_uint)(UNIT_NAME(log_buffer)* buff, unit_long_uint_t v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    unit_buffer_push_fmt_uint(&buff->buff, v);
    _unit_log_buffer_parse_format(buff);
    return 0;
}

int UNIT_NAME(log_buffer_add_fmt_double)(UNIT_NAME(log_buffer)* buff, double v) {
    if (!buff->state) {
        UNIT_ABORT();
    }
    unit_buffer_push_fmt_double(&buff->buff, v);
    _unit_log_buffer_parse_format(buff);
    return 0;
}
