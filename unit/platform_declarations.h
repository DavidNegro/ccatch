/* Forward declarations of the functions a platfrom must implement
*/


/* `unit_platform_on_run_aborted` is called when the run execution is aborted due to
      non-test-failure related error, like invalid parameters or unexpected
      test-running state.
*/
static int unit_platform_on_run_aborted(
   const char* error_description
);


/* unit_platform_on_subset_start is called before running a test case

   NOTE: if unit_test_runner_run_test_subcase is not called, the test is run
   in "exploratory mode" meaning that the subset_path variable may not point to
   the complete flow_path that will be executed
*/
static int unit_platform_on_subtest_start(
    const unit_test_registry_t* test,
    unit_flow_path_t* subtest_path,
    unit_size_t subtest_path_length);

/* <platform_on_log> is called when a log is produced

   TODO: add more details, explain that LOG, CHECK, EXPECT... all will end up
   calling this + the call is responsible from cleaning the log_buffer
*/
static int unit_platform_on_log(
   unit_log_buffer_t* buffer,
   const char* macro_name,
   int is_error
);

/*
*/
static int unit_platform_on_subtest_finished(
    const unit_test_registry_t* test,
    unit_flow_path_t* subtest_path,
    unit_size_t subtest_path_length,
    int with_error
);
