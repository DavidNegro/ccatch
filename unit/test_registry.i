/* Global buffer listing all the tests

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
typedef struct UNIT_NAME(test_registry) {
    const char* name;
    const char* tags;
    UNIT_NAME(case_fn) fn;
} UNIT_NAME(test_registry);

static UNIT_NAME(buffer) UNIT_NAME(registered_tests) = {0, 0, 0};

int UNIT_NAME(register_test)(UNIT_NAME(case_fn) fn, const char* fn_name, const char* tags) {
    UNIT_NAME(test_registry) registry;
    registry.name = fn_name;
    registry.tags = tags;
    registry.fn = fn;
    __test_tbuffer_ppush__(UNIT_NAME(test_registry), &UNIT_NAME(registered_tests), &registry);
    return 0;
}

static const UNIT_NAME(test_registry)* UNIT_NAME(get_test_registry)(size_t* elements) {
    *elements =  __test_tbuffer_size__(UNIT_NAME(test_registry), &UNIT_NAME(registered_tests));
    return __test_tbuffer_elements__(UNIT_NAME(test_registry), &UNIT_NAME(registered_tests));
}
