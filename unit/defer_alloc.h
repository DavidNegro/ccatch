// ---- scoped alloc and scoped defer ----

typedef struct unit_alloc_data_t {
    void* memory;
} unit_alloc_data_t;

typedef struct unit_defer_data_t {
    void* data;
    unit_defer_fn_t fn;
} unit_defer_data_t;

static unit_mutex_t _global_alloc_mutex;
static unit_buffer_t _global_alloc_stack;

static unit_mutex_t _global_defer_mutex;
static unit_buffer_t _global_defer_stack;

void* UNIT_NAME(scoped_alloc)(unit_size_t size) {
    void* result = UNIT_ALLOC(size);
    if (result == 0) {
        return result;
    }
    unit_alloc_data_t data;
    data.memory = result;
    unit_mutex_lock(&_global_alloc_mutex);
    unit_tbuffer_ppush(unit_alloc_data_t, &_global_alloc_stack, &data);
    unit_mutex_unlock(&_global_alloc_mutex);
    return result;
}

void UNIT_NAME(defer)(void* ptr, unit_defer_fn_t fn)  {
    unit_defer_data_t data;
    data.data = ptr;
    data.fn = fn;
    unit_mutex_lock(&_global_defer_mutex);
    unit_tbuffer_ppush(unit_defer_data_t, &_global_defer_stack, &data);
    unit_mutex_unlock(&_global_defer_mutex);
}

static void unit_defer_alloc_init(void) {
    unit_mutex_init(&_global_alloc_mutex);
    unit_mutex_init(&_global_defer_mutex);
}

static void unit_defer_alloc_deinit(void) {
    unit_mutex_deinit(&_global_alloc_mutex);
    unit_mutex_deinit(&_global_defer_mutex);
}

static void unit_defer_alloc_clean(void) {
    {
        // Run all the defered
        // TODO maybe we also need to catch exceptions here?
        size_t size = unit_tbuffer_size(unit_defer_data_t, &_global_defer_stack);
        if (size) {
            unit_defer_data_t* first = unit_tbuffer_elements(unit_defer_data_t, &_global_defer_stack);
            unit_defer_data_t* it = first + size -1;
            for (; it >= first; it--) {
                it->fn(it->data);
            }
            _global_defer_stack.size = 0;
        }
    }
    {
        // Run all scopped free
        size_t size = unit_tbuffer_size(unit_alloc_data_t, &_global_alloc_stack);
        if (size) {
            unit_alloc_data_t* first = unit_tbuffer_elements(unit_alloc_data_t, &_global_alloc_stack);
            unit_alloc_data_t* it = first + size -1;
            for (; it >= first; it--) {
                UNIT_FREE(it->memory);
            }
            _global_alloc_stack.size = 0;
        }
    }
}
