#include "query.h"

// pretty print queries
void print_query(log_t log_type, query_t *query) {
    print(log_type, "id: ");
    if (query->ignore_id) {
        print(log_type, "-");
    } else {
        print_key(log_type, &query->id);
    }
    print(log_type, "\n");

    print(log_type, "name: ");
    if (query->ignore_name) {
        print(log_type, "-");
    } else {
        print(log_type, "%s", query->name);
    }
    print(log_type, "\n");

    print(log_type, "size: ");
    if (query->ignore_size) {
        print(log_type, "-");
    } else {
        print(log_type, "%d", query->size);
    }
    print(log_type, "\n");
}

// pretty print results
void print_result(log_t log_type, query_result_t *result) {
    // print(log_type, "id: ");
    // print_key(log_type, &query->id);
    // print(log_type, "\n");
    print(log_type, "(name: %s, size: %d)\n", result->name, result->size);
}