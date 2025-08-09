
typedef struct unit_buffer_t {
    char* memory;
    unit_size_t size;
    unit_size_t reserved;
} unit_buffer_t;

#define __TEST_BUFFER_MIN_SIZE__ 512

static void unit_buffer_reserve(unit_buffer_t* buffer, unit_size_t size) {
    if (buffer->memory == 0) {
        unit_size_t new_capacity = UNIT_MAX(size, __TEST_BUFFER_MIN_SIZE__);
        char* new_memory = (char*)UNIT_ALLOC(new_capacity);
        if (!new_memory) {
            UNIT_ABORT();
        }
        buffer->memory = new_memory;
        buffer->reserved = new_capacity;
    }
    else if (buffer->size + size > buffer->reserved) {
        unit_size_t new_capacity = UNIT_MAX(buffer->size + size, buffer->size * 2);
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

static void unit_buffer_push(unit_buffer_t* buffer, const char* data, unit_size_t size) {
    unit_buffer_reserve(buffer, size);
    UNIT_MEMCPY(buffer->memory + buffer->size, data, size);
    buffer->size += size;
};

static void unit_buffer_resize(unit_buffer_t* buffer, unit_size_t new_size) {
    if (buffer->reserved < new_size) {
        unit_size_t diff = new_size - buffer->reserved;
        unit_buffer_reserve(buffer, diff);
    }
    buffer->size = new_size;
    return;
};

static void unit_buffer_deinit(unit_buffer_t* buffer) {
    if (buffer->memory) {
        UNIT_FREE(buffer->memory);
    }
};

#undef __TEST_BUFFER_MIN_SIZE__


// helper macros to use unit_buffer in a typed way (tbuffer)

#define unit_tbuffer_size(type, bufferptr) ((bufferptr)->size/sizeof(type))

#define unit_tbuffer_push(type, bufferptr, element)                             \
    do {                                                                        \
        type UNIT_TMPNAME(e) = (element);                                       \
        unit_buffer_push(bufferptr, &UNIT_TMPNAME(e), sizeof(type));            \
    } while(0)

#define unit_tbuffer_ppush(type, bufferptr, elementptr)                         \
    do {                                                                        \
        unit_buffer_push(bufferptr, (const char*)(elementptr), sizeof(type));   \
    } while(0)

#define unit_tbuffer_pop(type, bufferptr)                                       \
    do{                                                                         \
        (bufferptr)->size -= sizeof(type);                                      \
    } while(0)

#define unit_tbuffer_elements(type, bufferptr)                                  \
    ((type*) ((bufferptr)->memory))


#define unit_tbuffer_at(type, bufferptr, idx)                               \
    (((type*) ((bufferptr)->memory)) + (idx))

#define unit_tbuffer_top(type, bufferptr)                                   \
    (                                                                       \
        unit_tbuffer_elements(type, bufferptr) +                            \
        unit_tbuffer_size(type, bufferptr) - 1                              \
    )

// macros and functions to add formatted text to the buffer
#define unit_buffer_push_str(bufferptr, s) unit_buffer_push((bufferptr), (s), UNIT_STRLEN(s))

static unit_size_t _back_push_uint_unsafe(char* buffer, unit_long_uint_t v) {
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
    unit_size_t result = 0;
    while (v) {
        unit_long_uint_t n = v%100;
        v = v/100;
        buffer-=2;
        UNIT_MEMCPY(buffer, &NUM_MAP[n*2], 2);
        result += 2;
    }
    if (result == 0) {
        *(buffer - 1) = '0';
        return 1;
    }
    if (*buffer == '0') {
        result-=1;
    }
    return result;
}

static void unit_buffer_push_fmt_uint(unit_buffer_t* buffer, unit_long_uint_t v) {
    char TMP[20];
    unit_size_t sz = _back_push_uint_unsafe(TMP + 20, v);
    unit_buffer_push(buffer, TMP + 20 -sz, sz);
}

static void unit_buffer_push_fmt_int(unit_buffer_t* buffer, unit_long_int_t v) {
    char TMP[21];
    if (v >= 0) {
        unit_size_t sz = _back_push_uint_unsafe(TMP + 21, v);
        unit_buffer_push(buffer, TMP + 21 -sz, sz);
    }
    else {
        unit_size_t sz = _back_push_uint_unsafe(TMP + 21, -v);
        *(TMP + 21 -sz - 1) = '-';
        unit_buffer_push(buffer, TMP + 21 -sz - 1, sz + 1);
    }
}

static void unit_buffer_push_fmt_double(unit_buffer_t* buffer, double v) {
    char TMP[24];
    int res = unit_fpconv_dtoa(v, TMP);
    unit_buffer_push(buffer, TMP, (unit_size_t)res);
}
