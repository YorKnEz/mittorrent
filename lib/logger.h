#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#define ENABLE_DEBUG  0
#define ENABLE_ERRORS 0

typedef enum { LOG, LOG_ERROR, LOG_DEBUG } log_t;

void print(log_t log_type, const char* fmt, ...);

#endif
