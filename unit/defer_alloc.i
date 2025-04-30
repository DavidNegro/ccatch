// ---- scoped alloc and scoped defer ----

typedef struct UNIT_NAME(alloc_data) {
    void* memory;
} UNIT_NAME(alloc_data);

typedef struct UNIT_NAME(defer_data) {
    void* data;
    UNIT_NAME(defer_fn) fn;
} UNIT_NAME(defer_data);

static UNIT_NAME(mutex) UNIT_NAME(alloc_mutex);
static UNIT_NAME(buffer) UNIT_NAME(alloc_stack);

static UNIT_NAME(mutex) UNIT_NAME(defer_mutex);
static UNIT_NAME(buffer) UNIT_NAME(defer_stack);

void* UNIT_NAME(scoped_alloc)(size_t size) {
    void* result = UNIT_ALLOC(size);
    if (result == 0) {
        return result;
    }
    UNIT_NAME(alloc_data) data;
    data.memory = result;
    UNIT_NAME(mutex_lock)(&UNIT_NAME(alloc_mutex));
    __test_tbuffer_ppush__(UNIT_NAME(alloc_data), &UNIT_NAME(alloc_stack), &data);
    UNIT_NAME(mutex_unlock)(&UNIT_NAME(alloc_mutex));
    return result;
}

void UNIT_NAME(defer)(void* ptr, UNIT_NAME(defer_fn) fn)  {
    UNIT_NAME(defer_data) data;
    data.data = ptr;
    data.fn = fn;
    UNIT_NAME(mutex_lock)(&UNIT_NAME(defer_mutex));
    __test_tbuffer_ppush__(UNIT_NAME(defer_data), &UNIT_NAME(defer_stack), &data);
    UNIT_NAME(mutex_unlock)(&UNIT_NAME(defer_mutex));
}

static void UNIT_NAME(defer_alloc_init)() {
    UNIT_NAME(mutex_init)(&UNIT_NAME(alloc_mutex));
    UNIT_NAME(mutex_init)(&UNIT_NAME(defer_mutex));
}

static void UNIT_NAME(defer_alloc_deinit)() {
    UNIT_NAME(mutex_deinit)(&UNIT_NAME(alloc_mutex));
    UNIT_NAME(mutex_deinit)(&UNIT_NAME(defer_mutex));
}

static void UNIT_NAME(defer_alloc_clean)() {
    {
        // Run all the defered
        // TODO maybe we also need to catch exceptions here?
        size_t size = __test_tbuffer_size__(UNIT_NAME(defer_data), &UNIT_NAME(defer_stack));
        if (size) {
            UNIT_NAME(defer_data)* first = __test_tbuffer_elements__(UNIT_NAME(defer_data), &UNIT_NAME(defer_stack));
            UNIT_NAME(defer_data)* it = first + size -1;
            for (; it >= first; it--) {
                it->fn(it->data);
            }
            UNIT_NAME(defer_stack).size = 0;
        }
    }
    {
        // Run all scopped free
        size_t size = __test_tbuffer_size__(UNIT_NAME(alloc_data), &UNIT_NAME(alloc_stack));
        if (size) {
            UNIT_NAME(alloc_data)* first = __test_tbuffer_elements__(UNIT_NAME(alloc_data), &UNIT_NAME(alloc_stack));
            UNIT_NAME(alloc_data)* it = first + size -1;
            for (; it >= first; it--) {
                UNIT_FREE(it->memory);
            }
            UNIT_NAME(alloc_stack).size = 0;
        }
    }
}
