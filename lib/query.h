#ifndef QUERY_H
#define QUERY_H

#include "key.h"

// queriable parameters of a file
// server will match all files that have the same id, name AND size
typedef struct {
    // fields to ignore in query
    // if we want to query only by name, then ignore_id = ingore_size = 1
    int32_t ignore_id, ignore_name, ignore_size;
    key2_t id;
    char name[512];
    uint64_t size;
} query_t;

typedef struct {
    key2_t id;
    char name[512];
    uint64_t size;
} query_result_t;

// pretty print queries
void print_query(query_t *query);

// pretty print results
void print_result(query_result_t *result);

#endif