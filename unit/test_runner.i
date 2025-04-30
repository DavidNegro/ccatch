/* Runs tests

    Types:
        - <test_registry>
            struct containing all the information associated with a test

    Functions:
        - int <register_test>(<case_fn> fn, const char* fn_name, const char* tags)
            Add a new element to the list of registered tests

        - <test_registry>* <get_test_registry>(size_t* elements)
            Returns the list of registered tests. the argument elements is set
            to the number of registered tests

*/

static int UNIT_NAME(has_error_flag) = 0;

int UNIT_NAME(has_error) (void) {
    return UNIT_NAME(has_error_flag);
}

void UNIT_NAME(log_buffer_log)(UNIT_NAME(log_buffer)* buff, const char* macro_name, int is_error) {
    if (buff->state != 0) {
        UNIT_ABORT(); // incomplete format string
        return;
    }
    UNIT_NAME(platform_on_log)(buff, macro_name, is_error);
    if (is_error) {
        UNIT_NAME(has_error_flag) = 1;
    }
}


static int UNIT_NAME(test_runner_run_test_all_subcases)(const UNIT_NAME(test_registry)* reg) {
    int r = 0;
    size_t num_cases = 0;
    size_t num_failed_cases = 0;
    size_t num_flow = 0;
    UNIT_NAME(flow_path)* subcase_flow;
    UNIT_NAME(subcase_tracker_reset)();
    do {
        UNIT_NAME(has_error_flag) = 0;
        {
            subcase_flow = UNIT_NAME(subcase_tracker_get_currnet_flow)(&num_flow);
            int res = UNIT_NAME(platform_on_subtest_start)(reg, subcase_flow, num_flow);
            if (res != 0) {
                return res;
            }
        }
        int result = UNIT_RUNEXCEPT(reg->fn);
        UNIT_NAME(defer_alloc_clean)();
        result |= UNIT_NAME(has_error)();
        {
            subcase_flow = UNIT_NAME(subcase_tracker_get_currnet_flow)(&num_flow);
            int res = UNIT_NAME(platform_on_subtest_finished)(reg, subcase_flow, num_flow, result);
            if (res != 0) {
                return res;
            }
        }
        r |= result;
        num_cases++;
    } while(UNIT_NAME(subcase_tracker_next_subcase)());
    return r;
}

static int UNIT_NAME(test_runner_run_test_subcase)(const UNIT_NAME(test_registry)* reg) {
    // todo
}

static void UNIT_NAME(init)() {
    UNIT_NAME(defer_alloc_init)();
}

static void UNIT_NAME(deinit)() {
    UNIT_NAME(defer_alloc_deinit)();
}

