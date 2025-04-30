
static int UNIT_NAME(platform_on_log)(struct UNIT_NAME(log_buffer)* buffer, const char* macro_name, int is_error);
static int UNIT_NAME(platform_on_subtest_start)(struct UNIT_NAME(test_registry)* test, struct UNIT_NAME(flow_path)* subtest_path, size_t subtest_path_length);
static int UNIT_NAME(platform_on_subtest_finished)(struct UNIT_NAME(test_registry)* test, struct UNIT_NAME(flow_path)* subtest_path, size_t subtest_path_length, int with_error);
