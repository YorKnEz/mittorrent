#ifndef ERROR_H
#define ERROR_H

/*
Error codes to be used by internal libs for user errors propagation
*/

#include "logger.h"

// macros used for error propagation
#define CHECK(stmt) (status = (stmt)) < 0
#define ERR(err_code, generic_error) print(LOG, "error: %s\n", err_code == ERR_GENERIC ? generic_error : get_err_msg(err_code))
#define ERR_GENERIC(generic_error) print(LOG, "error: %s\n", generic_error)

typedef enum {
    ERR_INVALID_CMD = -1000,  // enforce all err codes to be negative
    ERR_INVALID_PATH,
    ERR_DOWNLOAD_NOT_A_DIR,
    ERR_DOWNLOAD_FILE_EXISTS,
    ERR_DOWNLOAD_NOT_RUNNING,
    ERR_PATH_TOO_LARGE,
    ERR_CANNOT_OPEN,
    ERR_INVALID_FILE,
    ERR_FILE_NOT_FOUND,

    ERR_GENERIC = -1,        // return -1 and show a generic error message
    ERR_SUCCESS = 0,          // return 0
} error_t;

// get error message
const char* get_err_msg(error_t err_code);

#endif
