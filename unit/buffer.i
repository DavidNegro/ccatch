
typedef struct UNIT_NAME(buffer) {
    char* memory;
    size_t size;
    size_t reserved;
} UNIT_NAME(buffer);

#define __TEST_BUFFER_MIN_SIZE__ 512

static void UNIT_NAME(buffer_reserve)(UNIT_NAME(buffer)* buffer, size_t size) {
    if (buffer->memory == 0) {
        size_t new_capacity = __TEST_MAX__(size, __TEST_BUFFER_MIN_SIZE__);
        char* new_memory = (char*)UNIT_ALLOC(new_capacity);
        if (!new_memory) {
            UNIT_ABORT();
        }
        buffer->memory = new_memory;
        buffer->reserved = new_capacity;
    }
    else if (buffer->size + size > buffer->reserved) {
        size_t new_capacity = __TEST_MAX__(buffer->size + size, buffer->size * 2);
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

static void UNIT_NAME(buffer_push)(UNIT_NAME(buffer)* buffer, const char* data, size_t size) {
    UNIT_NAME(buffer_reserve)(buffer, size);
    UNIT_MEMCPY(buffer->memory + buffer->size, data, size);
    buffer->size += size;
};

static void UNIT_NAME(buffer_resize)(UNIT_NAME(buffer)* buffer, size_t new_size) {
    if (buffer->reserved < new_size) {
        size_t diff = new_size - buffer->reserved;
        UNIT_NAME(buffer_reserve)(buffer, diff);
    }
    buffer->size = new_size;
    return;
};

static void UNIT_NAME(buffer_deinit)(UNIT_NAME(buffer)* buffer) {
    if (buffer->memory) {
        UNIT_FREE(buffer->memory);
    }
};

#undef __TEST_BUFFER_MIN_SIZE__


// helper macros to use __test_buffer__ in a typed way (tbuffer)

#define __test_tbuffer_size__(type, bufferptr) ((bufferptr)->size/sizeof(type))

#define __test_tbuffer_push__(type, bufferptr, element)                         \
    do {                                                                        \
        type UNIT_TMPNAME(e) = (element);                                       \
        UNIT_NAME(buffer_push)(bufferptr, &UNIT_TMPNAME(e), sizeof(type));      \
    } while(0)

#define __test_tbuffer_ppush__(type, bufferptr, elementptr)                     \
    do {                                                                        \
        UNIT_NAME(buffer_push)(bufferptr, (const char*)(elementptr), sizeof(type));\
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
#define __test_buffer_push_str__(bufferptr, s) UNIT_NAME(buffer_push)((bufferptr), (s), UNIT_STRLEN(s))

static size_t UNIT_NAME(back_push_uint_unsafe)(char* buffer, uint64_t v) {
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

static void UNIT_NAME(buffer_push_fmt_uint)(UNIT_NAME(buffer)* buffer, uint64_t v) {
    char TMP[20];
    size_t sz = UNIT_NAME(back_push_uint_unsafe)(TMP + 20, v);
    UNIT_NAME(buffer_push)(buffer, TMP + 20 -sz, sz);
}

static void UNIT_NAME(buffer_push_fmt_int)(UNIT_NAME(buffer)* buffer, int64_t v) {
    char TMP[21];
    if (v >= 0) {
        size_t sz = UNIT_NAME(back_push_uint_unsafe)(TMP + 21, v);
        UNIT_NAME(buffer_push)(buffer, TMP + 21 -sz, sz);
    }
    else {
        size_t sz = UNIT_NAME(back_push_uint_unsafe)(TMP + 21, -v);
        *(TMP + 21 -sz - 1) = '-';
        UNIT_NAME(buffer_push)(buffer, TMP + 21 -sz - 1, sz + 1);
    }
}

static void UNIT_NAME(buffer_push_fmt_double)(UNIT_NAME(buffer)* buffer, double v) {
    char TMP[24];
    int res = UNIT_NAME(fpconv_dtoa)(v, TMP);
    UNIT_NAME(buffer_push)(buffer, TMP, (size_t)res);
}
