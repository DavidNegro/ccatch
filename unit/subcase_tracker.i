
enum {
    UNIT_NAME(flow_flag_has_next) = 1,
    UNIT_NAME(flow_flag_is_section) = 2,
};

typedef struct UNIT_NAME(flow_path) {
    size_t index;
    unsigned flags;
    const char* description;
} UNIT_NAME(flow_path);

typedef struct UNIT_NAME(section_tracking) {
    size_t flow_start;
    size_t current_section_number;
} UNIT_NAME(section_tracking);

static size_t UNIT_NAME(flow_index);
static int UNIT_NAME(scope_has_run_section);
static UNIT_NAME(buffer)  UNIT_NAME(flow_buffer);
static UNIT_NAME(buffer)  UNIT_NAME(section_stack);

size_t UNIT_NAME(generate)(const char* name, size_t n) {
    size_t max_flow_index = __test_tbuffer_size__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer));
    size_t current_idx = UNIT_NAME(flow_index)++;
    if (max_flow_index <= current_idx) {
        UNIT_NAME(flow_path) path;
        path.description = name;
        path.index = 0;
        path.flags = 0;
        if (n > 0) {
            path.flags |= UNIT_NAME(flow_flag_has_next);
        }
        __test_tbuffer_ppush__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer), &path);
        return 0;
    }
    else {
        UNIT_NAME(flow_path)* path = __test_tbuffer_at__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer), current_idx);
        path->description = name;
        path->flags = 0;
        if (n != path->index + 1) {
            path->flags |= UNIT_NAME(flow_flag_has_next);
        }
        return path->index;
    }
}

int UNIT_NAME(section_start)(const char* name) {
    UNIT_NAME(section_tracking)* tracking;
    UNIT_NAME(flow_path)* path;

    if (UNIT_NAME(scope_has_run_section)) {
        tracking = __test_tbuffer_top__(UNIT_NAME(section_tracking), &UNIT_NAME(section_stack));
        path = __test_tbuffer_at__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer), tracking->flow_start);
    }
    else {
        UNIT_NAME(section_tracking) t;
        t.flow_start = UNIT_NAME(flow_index);
        t.current_section_number = 0;
        __test_tbuffer_ppush__(UNIT_NAME(section_tracking), &UNIT_NAME(section_stack), &t);
        tracking = __test_tbuffer_top__(UNIT_NAME(section_tracking), &UNIT_NAME(section_stack));

        size_t max_flow_index = __test_tbuffer_size__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer));
        size_t current_idx = UNIT_NAME(flow_index)++;
        if (max_flow_index <= current_idx) {
            UNIT_NAME(flow_path) p;
            p.description = name;
            p.index = 0;
            p.flags = UNIT_NAME(flow_flag_is_section);
            __test_tbuffer_ppush__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer), &p);
        }
        path = __test_tbuffer_at__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer), current_idx);
    }

    if (tracking->current_section_number == path->index) {
        // we need to run the section
        UNIT_NAME(scope_has_run_section) = 0;
        path->description = name;
        path->flags = UNIT_NAME(flow_flag_is_section);
        return 0;
    }
    else {
        // skip
        if (tracking->current_section_number > path->index) {
            path->flags |= UNIT_NAME(flow_flag_has_next);
        }
        tracking->current_section_number++;
        UNIT_NAME(scope_has_run_section) = 1;
        return 1;
    }
}

int UNIT_NAME(section_end)() {
    if (UNIT_NAME(scope_has_run_section)) {
        __test_tbuffer_pop__(UNIT_NAME(section_tracking), &UNIT_NAME(section_stack));
    }
    UNIT_NAME(section_tracking)* tracking = __test_tbuffer_top__(UNIT_NAME(section_tracking), &UNIT_NAME(section_stack));
    tracking->current_section_number++;
    UNIT_NAME(scope_has_run_section) = 1;
    return 1;
}

static void UNIT_NAME(subcase_tracker_reset)() {
    UNIT_NAME(section_stack).size = 0;
    UNIT_NAME(flow_buffer).size = 0;
    UNIT_NAME(scope_has_run_section) = 0;
    UNIT_NAME(flow_index) = 0;
}


static int UNIT_NAME(subcase_tracker_next_subcase)() {
    size_t size = __test_tbuffer_size__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer));
    UNIT_NAME(flow_path)* paths = __test_tbuffer_elements__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer));
    for (size_t i = size - 1; i < size; i--) {
        UNIT_NAME(flow_path)* p = paths + i;
        if (p->flags & UNIT_NAME(flow_flag_has_next)) {
            p->index += 1;
            p->flags = p->flags ^ UNIT_NAME(flow_flag_has_next);
            UNIT_NAME(flow_buffer).size = (i + 1) * sizeof(UNIT_NAME(flow_path));
            // reset all the variables to start the test
            UNIT_NAME(section_stack).size = 0;
            UNIT_NAME(scope_has_run_section) = 0;
            UNIT_NAME(flow_index) = 0;
            return 1;
        }
    }
    return 0;
}

static UNIT_NAME(flow_path)* UNIT_NAME(subcase_tracker_get_currnet_flow)(size_t* size) {
    *size = __test_tbuffer_size__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer));
    return __test_tbuffer_elements__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer));
}

static void UNIT_NAME(subcase_tracker_set_current_flow)(size_t size, size_t* idxs) {
    UNIT_NAME(buffer_resize)(&UNIT_NAME(flow_buffer), size*sizeof(UNIT_NAME(flow_path)));
    UNIT_NAME(flow_path)* path = __test_tbuffer_elements__(UNIT_NAME(flow_path), &UNIT_NAME(flow_buffer));
    for(size_t i = 0; i < size; i++) {
        path[i].index = idxs[i];
        path[i].flags = 0;
        path[i].description = "";
        // reset all the variables to start the test
        UNIT_NAME(section_stack).size = 0;
        UNIT_NAME(scope_has_run_section) = 0;
        UNIT_NAME(flow_index) = 0;
    }
}
