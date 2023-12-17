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

// all commands and standalone args of commands correspond to a cmd type
// for example `download -i` is a standalone command so it has the CMD_DOWNLOAD assigned to it
// `search -i id -n name -s size` is not a standalone command, but it still has CMD_SEARCH assigned to it
typedef enum {
    // client commands
    CMD_TRACKER_START,
    CMD_TRACKER_STOP,
    CMD_TRACKER_STABILIZE,
    CMD_TRACKER_STATE,
    CMD_TRACKER_HELP,
    CMD_SEARCH,
    CMD_SEARCH_HELP,
    CMD_DOWNLOAD,
    CMD_DOWNLOAD_LIST,
    CMD_DOWNLOAD_PAUSE,
    CMD_DOWNLOAD_HELP,
    CMD_UPLOAD,
    CMD_UPLOAD_HELP,

    // server commands
    CMD_SERVER_START,
    CMD_SERVER_STOP,
    CMD_SERVER_HELP,
    CMD_DB_LIST_PEERS,
    CMD_DB_LIST_UPLOADS,
    CMD_DB_HELP,

    // common commands
    CMD_HELP,
    CMD_CLEAR,
    CMD_QUIT,
    CMD_UNKNOWN,
    CMD_ERROR,
} cmd_type_t;

typedef struct {
    cmd_type_t cmd_type;        // the type of a standalone command, otherwise unknown
    int32_t excl;               // is flag exclusive or not
    const char *short_name;     // short name of a flag -f
    const char *long_name;      // long name of a flag --flag
    const char *placeholder;    // if the flag should take a value, this is the name of the value
    const char *desc;           // description of cmd
} cmd_arg_t;

// TODO: maybe use cmd_type to define groups of commands
// for example all flags of type CMD_GROUP can be combined, but no other arg can make part of it
// what about collisions?

// TODO: remove exclusive
typedef struct {
    cmd_type_t cmd_type;        // the default type of the command, can be unknown if the command contains only standalone commands
    const char *cmd_name;       // name of command
    const char *desc;           // description of command
    uint32_t args_size;         // the number of args that are defined for it
    cmd_arg_t args[10];         // the args
} cmd_t;

typedef struct {
    const char *prolog;
    const char *desc;
    uint32_t size;
    cmd_t list[10];
} cmds_t;

int32_t cmd_parse(parsed_cmd_t *cmd, char *buf);

void print_parsed_cmd(log_t log_type, parsed_cmd_t *cmd);

void print_cmd_help(log_t log_type, cmd_t *cmd);

void cmds_get_cmd_type(cmds_t *cmds, parsed_cmd_t *cmd, cmd_type_t *cmd_type);

void print_cmds_help(log_t log_type, cmds_t *cmds);

#endif
