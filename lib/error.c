#include "error.h"

const char* get_err_msg(error_t err_code) {
    switch (err_code) {
    case ERR_INVALID_PATH: return "invalid path";
    case ERR_DOWNLOAD_NOT_A_DIR:    return "not a directory path";
    case ERR_DOWNLOAD_FILE_EXISTS:  return "file already exists";
    case ERR_DOWNLOAD_NOT_RUNNING:  return "download is not running";
    case ERR_PATH_TOO_LARGE:        return "file path is too large";
    case ERR_CANNOT_OPEN:           return "cannot open file";
    case ERR_INVALID_FILE:          return "invalid file format";
    case ERR_FILE_NOT_FOUND:        return "file not found";
    default:                        return "unknown error";
    }
}
