#include "query.h"

// pretty print queries
void print_query(query_t *query) {
    print(LOG_DEBUG, "id: ");
    if (query->ignore_id) {
        print(LOG_DEBUG, "-");
    } else {
        print_key(&query->id);
    }
    print(LOG_DEBUG, "\n");

    print(LOG_DEBUG, "name: ");
    if (query->ignore_name) {
        print(LOG_DEBUG, "-");
    } else {
        print(LOG_DEBUG, "%s", query->name);
    }
    print(LOG_DEBUG, "\n");

    print(LOG_DEBUG, "size: ");
    if (query->ignore_size) {
        print(LOG_DEBUG, "-");
    } else {
        print(LOG_DEBUG, "%d", query->size);
    }
    print(LOG_DEBUG, "\n");
}

// pretty print results
void print_result(query_result_t *result) {
    // print(LOG_DEBUG, "id: ");
    // print_key(&query->id);
    // print(LOG_DEBUG, "\n");
    print(LOG_DEBUG, "name: %s\n", result->name);
    print(LOG_DEBUG, "size: %d\n", result->size);
}