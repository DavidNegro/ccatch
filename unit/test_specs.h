
/* This file implements an expression to filter tests based test names and tags

   The grammar of the test filters is the following:

    SPEC := AND_EXPR (',' AND_EXPR)*
    AND_EXPR := UNIT_EXPR+
    UNIT_EXPR := '~'? ATOM_EXPR
    ATOM_EXPR := '(' SPEC ')' | IDENT
    IDENT := '[' TAG_WILDCARD ']' | ''' TEST_NAME_WILDCARD ''' |
             '"' TEST_NAME_WILDCARD '"' | TEST_NAME_WILDCARD

    Where:
      - ',' combines filters like an OR expression
      - concatenating filters combines the filters like an AND expression
      - '~' negates the filter
      - '[' TAG_WILDCARD ']' matches if any of the tags matches the name pattern
      - TEST_NAME_WILDCARD matches if the test name matches the pattern
      - TAG_WILDCARD and TEST_NAME_WILDCARD are normal strings that may
        contain '*' characters. Each '*' character mach any number of
        characters

    Types:
        - unit_test_specs_t
            The type representing a test filter

    Functions:
        - int unit_test_specs_init(unit_test_specs_t* specs, const char* str_begin, const char* str_end)
            Parse the filter delimited by the pointers str_begin, str_end and
            initializes a <test_specs> object implementing the filter.
            Returns -1 on error
            Returns 0 on success

        - int unit_test_specs_match(unit_test_specs_t* specs, const unit_test_registry_t* reg)
            Returns 1 if the given test matches the given filter
            Returns 0 in other cases
*/

enum {
    unit_test_specs_match_node_kind_test_name,
    unit_test_specs_match_node_kind_test_tag,
    unit_test_specs_match_node_kind_and,
    unit_test_specs_match_node_kind_or,
};

typedef struct unit_test_specs_match_node_t {
    int kind;
    int not;
    const char* str_start;
    const char* str_end;
    int children_begin;
    int children_next;
} unit_test_specs_match_node_t;

typedef struct unit_test_specs_t {
    unit_buffer_t buffer;
    unit_size_t node_index;
} unit_test_specs_t;


static unit_buffer_t _unit_test_specs_error_buffer = {0, 0, 0};
static const char* _unit_test_specs_error_pos = 0;

static const char* unit_test_specs_get_parse_error_description(const char* str) {
    if (str) {
        unit_buffer_push_str(&_unit_test_specs_error_buffer, " at position ");
        unit_buffer_push_fmt_uint(&_unit_test_specs_error_buffer, (unit_long_uint_t)(_unit_test_specs_error_pos - str));
    }
    unit_tbuffer_push(char, &_unit_test_specs_error_buffer, '\0');
    return (const char*)_unit_test_specs_error_buffer.memory;
}

static const char* unit_test_specs_get_parse_error_pos(void) {
    return _unit_test_specs_error_pos;
}

static void unit_test_specs_set_parse_error(const char* error_description, const char* error_pos) {
    _unit_test_specs_error_buffer.size = 0;
    unit_buffer_push_str(&_unit_test_specs_error_buffer, error_description);
    _unit_test_specs_error_pos = error_pos;
}

// forward declarations
static int unit_test_specs_parse_spec(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str);
static int unit_test_specs_parse_and_expr(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str);
static int unit_test_specs_parse_unit_expr(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str);
static int unit_test_specs_parse_atom_expr(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str);
static int unit_test_specs_parse_ident(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str);

static int unit_test_specs_parse_spec(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str) {
    const char* it = str_begin;
    int res = unit_test_specs_parse_and_expr(node_buffer, it, str_end, &it);
    if (res == -1) {
        return -1;
    }

    if (it == str_end || *it != ',') {
        *out_str = it;
        return res;
    }

    // allocate a node and consume all or expressions
    unit_test_specs_match_node_t result_node;
    result_node.kind = unit_test_specs_match_node_kind_or;
    result_node.not = 0;
    result_node.children_begin = res;
    result_node.children_next = -1;
    int last_node = res;
    res = (int)unit_tbuffer_size(unit_test_specs_match_node_t, node_buffer);
    unit_tbuffer_ppush(unit_test_specs_match_node_t, node_buffer, &result_node);

    while(it != str_end && *it == ',') {
        it++;
        if (it == str_end) {
            unit_test_specs_set_parse_error("\",\" character found at the end of the filter expression", str_end);
            return -1;
        }
        int res_cont = unit_test_specs_parse_and_expr(node_buffer, it, str_end, &it);
        if (res_cont == -1) {
            return -1;
        }
        unit_test_specs_match_node_t* node =
            unit_tbuffer_at(unit_test_specs_match_node_t, node_buffer, last_node);
        node->children_next = res_cont;
        last_node = res_cont;
    }
    *out_str = it;
    return res;
}

static int unit_test_specs_parse_and_expr(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str) {
    const char* it = str_begin;
    int res = unit_test_specs_parse_unit_expr(node_buffer, it, str_end, &it);
    if (res == -1) {
        return -1;
    }

    if (it == str_end || *it == ')' || *it == ',') {
        *out_str = it;
        return res;
    }

    // allocate a node and consume all and expressions
    unit_test_specs_match_node_t result_node;
    result_node.kind = unit_test_specs_match_node_kind_and;
    result_node.not = 0;
    result_node.children_begin = res;
    result_node.children_next = -1;
    int last_node = res;
    res = (int)unit_tbuffer_size(unit_test_specs_match_node_t, node_buffer);
    unit_tbuffer_ppush(unit_test_specs_match_node_t, node_buffer, &result_node);

    while(it != str_end && *it != ')' || *it != ',') {
        int res_cont = unit_test_specs_parse_unit_expr(node_buffer, it, str_end, &it);
        if (res_cont == -1) {
            break;
        }
        unit_test_specs_match_node_t* node =
            unit_tbuffer_at(unit_test_specs_match_node_t, node_buffer, last_node);
        node->children_next = res_cont;
        last_node = res_cont;
    }
    *out_str = it;
    return res;
}

static int unit_test_specs_parse_unit_expr(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str) {
    if (str_begin == str_end) {
        return -1;
    }

    const char* it = str_begin;
    int not = 0;
    if (*it == '~') {
        not = 1;
        it++;
    }

    int res = unit_test_specs_parse_atom_expr(node_buffer, it, str_end, &it);
    if (res == -1) {
        return -1;
    }
    if (not) {
        unit_test_specs_match_node_t* node =
            unit_tbuffer_at(unit_test_specs_match_node_t, node_buffer, res);
        node->not = 1;
    }
    *out_str = it;
    return res;
}

static int unit_test_specs_parse_atom_expr(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str) {
    // '(' SPEC ')' | IDENT
    if (str_begin == str_end) {
        return -1;
    }

    const char* it = str_begin;
    if (*it == '(') {
        it++;
        int res = unit_test_specs_parse_spec(node_buffer, it, str_end, &it);
        if (res == -1) {
            return -1;
        }
        if (*it != ')') {
            unit_test_specs_set_parse_error("\")\" was expected at this point", it);
            return -1;
        }
        it++;

        *out_str = it;
        return res;
    }
    else {
        return unit_test_specs_parse_ident(node_buffer, str_begin, str_end, out_str);
    }
}

static int unit_test_specs_parse_ident(unit_buffer_t* node_buffer, const char* str_begin, const char* str_end, const char** out_str) {
    if (str_begin == str_end) {
        return -1;
    }
    const char* it = str_begin;
    char delim_end = '\0';
    int ident_kind = unit_test_specs_match_node_kind_test_name;
    if (*it == '\"' || *it == '\'') {
        delim_end = *it;
        it++;
    }
    else if (*it == '[') {
        delim_end = ']';
        ident_kind = unit_test_specs_match_node_kind_test_tag;
        it++;
    }
    const char* ident_begin = it;
    while(*it != delim_end && *it != '(' && *it != ')' && *it != '[' && *it != '~' && *it != ',' && it != str_end )  {
        it++;
    }
    if (delim_end && (it == str_end || *it != delim_end)) {
        char error_msg[] = "\"_\" was expected at this point";
        error_msg[1] = delim_end;
        unit_test_specs_set_parse_error(error_msg, it);
        return -1;
    }
    const char* ident_end = it;
    if (delim_end) {
        it++;
    }
    unit_test_specs_match_node_t node;
    node.kind = ident_kind;
    node.not = 0;
    node.str_start = ident_begin;
    node.str_end = ident_end;
    node.children_begin = -1;
    node.children_next = -1;
    int last_node = (int)unit_tbuffer_size(unit_test_specs_match_node_t, node_buffer);
    unit_tbuffer_ppush(unit_test_specs_match_node_t, node_buffer, &node);
    *out_str = it;
    return last_node;
}

static int unit_test_specs_init(unit_test_specs_t* specs, const char* str_begin, const char* str_end) {
    // reset error buffer
    _unit_test_specs_error_buffer.size = 0;

    UNIT_MEMSET(specs, 0, sizeof(unit_test_specs_t));
    const char* it;
    int res = unit_test_specs_parse_spec(&specs->buffer, str_begin, str_end, &it);
    if (res == -1) {
        return -1;
    }
    if (it != str_end &&  _unit_test_specs_error_buffer.size == 0) {
        unit_test_specs_set_parse_error("Parsing finished at this point but an end-of-line was not found", it);
        return -1;
    }
    specs->node_index = (unit_size_t)res;
    return 0;
}

// wildcard matching

static int unit_wildcard_match(const char* wildcard_begin, const char* wildcard_end, const char* str_begin, const char* str_end);
static int unit_wildcard_any_match(const char* wildcard_begin, const char* wildcard_end, const char* str_begin, const char* str_end);


static int unit_wildcard_match(const char* wildcard_begin, const char* wildcard_end, const char* str_begin, const char* str_end) {
    while (wildcard_begin != wildcard_end && str_begin != str_end) {
        if (*wildcard_begin == '*') {
            return unit_wildcard_any_match(wildcard_begin + 1, wildcard_end, str_begin, str_end);
        }
        if (*wildcard_begin != *str_begin) {
            return 0;
        }
        wildcard_begin++;
        str_begin++;
    }
    if (wildcard_begin != wildcard_end || str_begin != str_end) {
        return 0;
    }
    return 1;
}

static int unit_wildcard_any_match(const char* wildcard_begin, const char* wildcard_end, const char* str_begin, const char* str_end) {
    if (wildcard_begin == wildcard_end) {
        return 1;
    }
    for (const char* it = str_begin; it != str_end; it++) {
        if (unit_wildcard_match(wildcard_begin, wildcard_end, it, str_end)) {
            return 1;
        }
    }
    return 0;
}

// test_specs match

static int unit_test_specs_match_node(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg);
static int unit_test_specs_match_and(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg);
static int unit_test_specs_match_or(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg);
static int unit_test_specs_match_test_name(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg);
static int unit_test_specs_match_test_tag(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg);

static int unit_test_specs_match_node(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg) {
    unit_test_specs_match_node_t* match_node =
        unit_tbuffer_at(unit_test_specs_match_node_t, buffer, node);
    int not = match_node->not;
    int result;
    switch (match_node->kind) {
    case unit_test_specs_match_node_kind_test_name:
        result = unit_test_specs_match_test_name(buffer, node, reg);
        break;
    case unit_test_specs_match_node_kind_test_tag:
        result = unit_test_specs_match_test_tag(buffer, node, reg);
        break;
    case unit_test_specs_match_node_kind_and:
        result = unit_test_specs_match_and(buffer, match_node->children_begin, reg);
        break;
    case unit_test_specs_match_node_kind_or:
        result = unit_test_specs_match_or(buffer, match_node->children_begin, reg);
        break;
    default:
        UNIT_ABORT();
        break;
    }
    if (not) {
        return !result;
    }
    return result;
}

static int unit_test_specs_match_and(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg) {
    while (node != -1) {
        unit_test_specs_match_node_t* match_node =
            unit_tbuffer_at(unit_test_specs_match_node_t, buffer, node);
        int res = unit_test_specs_match_node(buffer, node, reg);
        if (res == 0) {
            return 0;
        }
        node = match_node->children_next;
    }
    return 1;
}

static int unit_test_specs_match_or(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg) {
    while (node != -1) {
        unit_test_specs_match_node_t* match_node =
            unit_tbuffer_at(unit_test_specs_match_node_t, buffer, node);
        int res = unit_test_specs_match_node(buffer, node, reg);
        if (res == 1) {
            return 1;
        }
        node = match_node->children_next;
    }
    return 0;
}

static int unit_test_specs_match_test_name(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg) {
    unit_test_specs_match_node_t* match_node =
        unit_tbuffer_at(unit_test_specs_match_node_t, buffer, node);
    return unit_wildcard_match(match_node->str_start, match_node->str_end, reg->name, reg->name + UNIT_STRLEN(reg->name));
}

static int unit_test_specs_match_test_tag(unit_buffer_t* buffer, int node, const unit_test_registry_t* reg) {
    unit_test_specs_match_node_t* match_node =
        unit_tbuffer_at(unit_test_specs_match_node_t, buffer, node);

    unit_test_tag_iterator_t it = { 0 };
    const char *tag_begin;
    const char *tag_end;
    while(unit_test_tag_iterate(reg, &it, &tag_begin, &tag_end)) {
        if (unit_wildcard_match(match_node->str_start, match_node->str_end, tag_begin, tag_end)) {
            return 1;
        }
    }
    return 0;
}

static int unit_test_specs_match(unit_test_specs_t* specs, const unit_test_registry_t* reg) {
    return unit_test_specs_match_node(&specs->buffer, (int)specs->node_index, reg);
}
