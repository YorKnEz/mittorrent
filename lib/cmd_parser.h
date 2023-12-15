#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include <string.h>
#include <stdint.h>

#include "logger.h"
#include "error.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

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
} parsed_cmd_arg_t;

typedef struct {
    char *name;         // command name
    uint32_t args_size;
    parsed_cmd_arg_t args[10]; // args array, max 10
} parsed_cmd_t;

typedef struct {
    const char *short_name;
    const char *long_name;
    const char *placeholder;
    const char *desc;
} cmd_arg_t;

typedef struct {
    const char *cmd_name;
    const char *desc;
    uint32_t args_size;
    cmd_arg_t args[10];
    uint32_t exclusive[10];
} cmd_t;

int32_t cmd_parse(parsed_cmd_t *cmd, char *buf);

void print_cmd(log_t log_type, parsed_cmd_t *cmd);

void print_cmd_help(log_t log_type, cmd_t *cmd);

#endif
