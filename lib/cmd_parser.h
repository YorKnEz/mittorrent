#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include <string.h>
#include <stdint.h>

#include "logger.h"
#include "error.h"

/*
given a buffer like:
`cmd -flag1 value1 -flag2 value2 -flag3 value3`
it puts this data in a specialized structure and checks for errors
an input like
`cmd value1` will be considered invalid
*/

typedef struct {
    char *flag;         // flag name
    char *value;        // value assigned to flag, can be NULL (for a command like cmd -h)
} cmd_arg_t;

typedef struct {
    char *name;         // command name
    uint32_t args_size;
    cmd_arg_t args[10]; // args array, max 10
} cmd_t;

int32_t cmd_parse(cmd_t *cmd, char *buf);

void print_cmd(log_t log_type, cmd_t *cmd);

#endif
