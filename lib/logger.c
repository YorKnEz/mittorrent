#include "logger.h"

void print(log_t log_type, const char* fmt, ...) {
    if (log_type == LOG_DEBUG && ENABLE_DEBUG != 1) {
        return;
    }

    if (log_type == LOG_ERROR && ENABLE_ERRORS != 1) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    // if (log_type == LOG_DEBUG) {
    //     printf("[%d] ", getpid());
    // }

    vprintf(fmt, args);

    if (log_type == LOG_ERROR) {
        printf("errno: %s\n", strerror(errno));
    }
    
    fflush(stdout);
    
    va_end(args);
}
