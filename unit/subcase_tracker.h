
enum {
    unit_flow_flag_has_next = 1,
    unit_flow_flag_is_section = 2,
};

typedef struct unit_flow_path_t {
    unit_size_t index;
    unit_size_t flags;
    const char* description;
} unit_flow_path_t;

typedef struct unit_section_tracking_t {
    unit_size_t flow_start;
    unit_size_t current_section_number;
} unit_section_tracking_t;

static unit_size_t unit_flow_index;
static int unit_scope_has_run_section;
static unit_buffer_t  unit_flow_buffer;
static unit_buffer_t  unit_section_stack;

unit_size_t UNIT_NAME(generate)(const char* name, unit_size_t n) {
    unit_size_t max_flow_index = unit_tbuffer_size(unit_flow_path_t, &unit_flow_buffer);
    unit_size_t current_idx = unit_flow_index++;
    if (max_flow_index <= current_idx) {
        unit_flow_path_t path;
        path.description = name;
        path.index = 0;
        path.flags = 0;
        if (n > 0) {
            path.flags |= unit_flow_flag_has_next;
        }
        unit_tbuffer_ppush(unit_flow_path_t, &unit_flow_buffer, &path);
        return 0;
    }
    else {
        unit_flow_path_t* path = unit_tbuffer_at(unit_flow_path_t, &unit_flow_buffer, current_idx);
        path->description = name;
        path->flags = 0;
        if (n != path->index + 1) {
            path->flags |= unit_flow_flag_has_next;
        }
        return path->index;
    }
}

int UNIT_NAME(section_start)(const char* name) {
    unit_section_tracking_t* tracking;
    unit_flow_path_t* path;

    if (unit_scope_has_run_section) {
        tracking = unit_tbuffer_top(unit_section_tracking_t, &unit_section_stack);
        path = unit_tbuffer_at(unit_flow_path_t, &unit_flow_buffer, tracking->flow_start);
    }
    else {
        unit_section_tracking_t t;
        t.flow_start = unit_flow_index;
        t.current_section_number = 0;
        unit_tbuffer_ppush(unit_section_tracking_t, &unit_section_stack, &t);
        tracking = unit_tbuffer_top(unit_section_tracking_t, &unit_section_stack);

        unit_size_t max_flow_index = unit_tbuffer_size(unit_flow_path_t, &unit_flow_buffer);
        unit_size_t current_idx = unit_flow_index++;
        if (max_flow_index <= current_idx) {
            unit_flow_path_t p;
            p.description = name;
            p.index = 0;
            p.flags = unit_flow_flag_is_section;
            unit_tbuffer_ppush(unit_flow_path_t, &unit_flow_buffer, &p);
        }
        path = unit_tbuffer_at(unit_flow_path_t, &unit_flow_buffer, current_idx);
    }

    if (tracking->current_section_number == path->index) {
        // we need to run the section
        unit_scope_has_run_section = 0;
        path->description = name;
        path->flags = unit_flow_flag_is_section;
        return 0;
    }
    else {
        // skip
        if (tracking->current_section_number > path->index) {
            path->flags |= unit_flow_flag_has_next;
        }
        tracking->current_section_number++;
        unit_scope_has_run_section = 1;
        return 1;
    }
}

int UNIT_NAME(section_end)(void) {
    if (unit_scope_has_run_section) {
        unit_tbuffer_pop(unit_section_tracking_t, &unit_section_stack);
    }
    unit_section_tracking_t* tracking = unit_tbuffer_top(unit_section_tracking_t, &unit_section_stack);
    tracking->current_section_number++;
    unit_scope_has_run_section = 1;
    return 1;
}

static void unit_subcase_tracker_reset(void) {
    unit_section_stack.size = 0;
    unit_flow_buffer.size = 0;
    unit_scope_has_run_section = 0;
    unit_flow_index = 0;
}

static int unit_subcase_tracker_next_subcase(void) {
    size_t size = unit_tbuffer_size(unit_flow_path_t, &unit_flow_buffer);
    unit_flow_path_t* paths = unit_tbuffer_elements(unit_flow_path_t, &unit_flow_buffer);
    for (size_t i = size - 1; i < size; i--) {
        unit_flow_path_t* p = paths + i;
        if (p->flags & unit_flow_flag_has_next) {
            p->index += 1;
            p->flags = p->flags ^ unit_flow_flag_has_next;
            unit_flow_buffer.size = (i + 1) * sizeof(unit_flow_path_t);
            // reset all the variables to start the test
            unit_section_stack.size = 0;
            unit_scope_has_run_section = 0;
            unit_flow_index = 0;
            return 1;
        }
    }
    return 0;
}

static unit_flow_path_t* unit_subcase_tracker_get_currnet_flow(unit_size_t* size) {
    *size = unit_tbuffer_size(unit_flow_path_t, &unit_flow_buffer);
    return unit_tbuffer_elements(unit_flow_path_t, &unit_flow_buffer);
}

static void unit_subcase_tracker_set_current_flow(unit_size_t size, unit_size_t* idxs) {
    unit_buffer_resize(&unit_flow_buffer, size*sizeof(unit_flow_path_t));
    unit_flow_path_t* path = unit_tbuffer_elements(unit_flow_path_t, &unit_flow_buffer);
    for(unit_size_t i = 0; i < size; i++) {
        path[i].index = idxs[i];
        path[i].flags = 0;
        path[i].description = "";
    }
    // reset all the variables to start the test
    unit_section_stack.size = 0;
    unit_scope_has_run_section = 0;
    unit_flow_index = 0;
}
