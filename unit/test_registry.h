/* Global buffer listing all the tests

    Types:
        - unit_test_registry_t
            struct containing all the information associated with a test

    Functions:
        - int <register_test>(<case_fn> fn, const char* fn_name, const char* tags, unit_size_t line, const char* filename)
            Add a new element to the list of registered tests

        - unit_test_registry_t* <get_test_registry>(unit_size_t* elements)
            Returns the list of registered tests. the argument elements is set
            to the number of registered tests

        - int <test_tag_iterate>(<test_registry>* registry, <test_tag_iterator>* it, const char** begin, const char** end)
            Iterate throught all the tags of a given test. Use it as follows:

              <test_tag_iterator> it = { 0 };
              const char *tag_begin, *tag_end;
              while(<test_tag_iterate>(my_test, &it, &tag_begin, &tag_end)) {
                  // do something with the tag_begin..tag_end string fragment
              }

*/

// --- tags ---

enum {
    unit_test_flag_exclude_from_default = 1,
    unit_test_flag_should_fail = 4,
    unit_test_flag_may_fail = 8,
};

static int unit_parse_string_tags(const char* str, unsigned char* flags) {
    static const char* should_fail = "!shouldfail";
    static const unit_size_t should_fail_size = sizeof("!shouldfail") - 1;
    static const char* may_fail = "!mayfail";
    static const unit_size_t may_fail_size = sizeof("!mayfail") - 1;

    unsigned char result_flags = 0;
    while (*str != '\0') {
        if (*str != '[') {
            return 0;
        }
        str++;
        if (*str == '\0') {
            return 0;
        }
        const char* begin_tag = str;
        while (*str != ']') {
            if (*str == '\0') {
                return 0;
            }
            str++;
        }
        const char* end_tag = str;
        str++;
        if (begin_tag == end_tag) {
            return 0;
        }
        // check special flags
        if (*begin_tag == '.') {
            // tag starting with '.' -> exclude from default
            result_flags |= unit_test_flag_exclude_from_default;
        }
        else if (*begin_tag == '!') {
            unit_size_t tag_size = (unit_size_t)(end_tag - begin_tag);
            if (tag_size == should_fail_size && UNIT_MEMCMP(begin_tag, should_fail, should_fail_size) == 0) {
                result_flags |= unit_test_flag_should_fail;
            }
            else if (tag_size == may_fail_size && UNIT_MEMCMP(begin_tag, may_fail, may_fail_size) == 0) {
                result_flags |= unit_test_flag_may_fail;
            }
            else {
                return 0;
            }
        }
    }
    *flags = result_flags;
    return 1;
}

static int unit_create_filename_tag(const char* filename, const char** tag) {
    unit_size_t len = UNIT_STRLEN(filename);
    if (len == 0) {
        return 0;
    }
    const char* it = filename + len - 1;
    const char* filename_begin = 0;
    const char* filename_end = 0;
    while (it > filename) {
        if (*it == '.') {
            filename_end = it;
            break;
        }
        if (*it == '/' || *it == '\\') {
            filename_begin = it + 1;
            filename_end = filename + len;
            break;
        }
        it--;
    }
    if (filename_end == 0) {
        return 0;
    }
    if (filename_begin  == 0) {
        while (it > filename) {
            if (*it == '/' || *it == '\\') {
                filename_begin = it + 1;
                break;
            }
            it--;
        }
    }
    if (filename_begin == 0 || filename_begin == filename_end) {
        return 0;
    }
    unit_size_t base_name_len = (unit_size_t)(filename_end - filename_begin);
    char* tag_memory = (char*)UNIT_ALLOC(base_name_len + 2);
    *tag_memory = '#';
    UNIT_MEMCPY(tag_memory + 1, filename_begin, base_name_len);
    *(tag_memory + 1 + base_name_len) = '\0';
    *tag = tag_memory;
    return 1;
}

static const char* unit_tag_string_iter(const char* it, const char** begin, const char** end) {
    if (*it == '\0') {
        return 0;
    }
    if (*it != '[') {
        UNIT_ABORT(); // unexpected condition
    }
    it++;
    const char* result_begin = it;
    while (*it != ']') {
        if (*it == 0) {
            UNIT_ABORT(); // unexpected condition
        }
        it++;
    }
    const char* result_end = it;
    if (result_begin == result_end) {
        UNIT_ABORT(); // unexpected condition
    }
    it++;

    *begin = result_begin;
    *end = result_end;
    return it;
}

// This structure represents a test

struct unit_test_registry_t {
    const char* name;
    const char* tags;
    const char* filename;
    const char* filename_tag;
    unit_size_t line;
    unit_size_t flags;
    unit_case_fn_t fn;
};
static unit_buffer_t unit_registered_tests = {0, 0, 0};

int UNIT_NAME(register_test)(UNIT_NAME(case_fn) fn, const char* fn_name, const char* tags, unit_size_t line, const char* filename) {

    // check if tags is a valid tags string
    unsigned char flags = 0;
    int result = unit_parse_string_tags(tags, &flags);
    if (!result) {
       UNIT_ABORT(); // Invalid string
    }

    // create a file tag string
    const char* filename_tag = 0;
    unit_create_filename_tag(filename, &filename_tag);

    unit_test_registry_t registry;
    registry.name = fn_name;
    registry.tags = tags;
    registry.line = line;
    registry.filename = filename;
    registry.filename_tag = filename_tag;
    registry.flags = flags;
    registry.fn = fn;
    unit_tbuffer_ppush(unit_test_registry_t, &unit_registered_tests, &registry);
    return 0;
}

static const unit_test_registry_t* unit_get_test_registry(unit_size_t* elements) {
    *elements =  unit_tbuffer_size(unit_test_registry_t, &unit_registered_tests);
    return unit_tbuffer_elements(unit_test_registry_t, &unit_registered_tests);
}

typedef struct unit_test_tag_iterator_t {
    unit_size_t flag; // 0 -> starting, 1 -> visiting tags, 2 -> visiting file tag / end
    const char* it;
} unit_test_tag_iterator_t;

static int unit_test_tag_iterate(const unit_test_registry_t* test, unit_test_tag_iterator_t* it, const char** begin, const char** end) {
    if (it->flag == 0) {
        it->flag = 1;
        it->it = test->tags;
    }
    if (it->flag == 1) {
        const char* next_it = unit_tag_string_iter(it->it, begin, end);
        if (next_it) {
            it->it = next_it;
            return 1;
        }
        // finish visiting all regular tags, try to visit the filename tag
        it->flag = 2;
        it->it = 0;
        if (test->filename_tag) {
            *begin = test->filename_tag;
            *end = test->filename_tag + UNIT_STRLEN(test->filename_tag);
            return 1;
        }
    }
    return 0;
}
