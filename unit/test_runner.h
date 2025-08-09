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

static int unit_has_error_flag = 0;

int UNIT_NAME(has_error)(void) {
    return unit_has_error_flag;
}

void UNIT_NAME(log_buffer_log)(unit_log_buffer_t* buff, const char* macro_name, int is_error) {
    if (buff->state != 0) {
        UNIT_ABORT(); // incomplete format string
        return;
    }
    unit_platform_on_log(buff, macro_name, is_error);
    if (is_error) {
        unit_has_error_flag = 1;
    }
}


static int unit_test_runner_run_test_all_subcases(const unit_test_registry_t* reg) {
    int r = 0;
    unit_size_t num_cases = 0;
    unit_size_t num_flow = 0;
    unit_flow_path_t* subcase_flow;
    unit_subcase_tracker_reset();
    do {
        unit_has_error_flag = 0;
        {
            subcase_flow = unit_subcase_tracker_get_currnet_flow(&num_flow);
            int res = unit_platform_on_subtest_start(reg, subcase_flow, num_flow);
            if (res != 0) {
                return res;
            }
        }
        int result = UNIT_RUNEXCEPT(reg->fn);
        unit_defer_alloc_clean();
        result |= UNIT_NAME(has_error)();
        {
            subcase_flow = unit_subcase_tracker_get_currnet_flow(&num_flow);
            int res = unit_platform_on_subtest_finished(reg, subcase_flow, num_flow, result);
            if (res != 0) {
                return res;
            }
        }
        r |= result;
        num_cases++;
    } while(unit_subcase_tracker_next_subcase());
    return r;
}

static int unit_test_runner_run_test_subcase(const unit_test_registry_t* reg) {
    // todo
    UNIT_IGNORE_UNUSED(reg);
    return 0;
}

static void unit_init(void) {
    unit_defer_alloc_init();
}

static void unit_deinit(void) {
    unit_defer_alloc_deinit();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
