#include "cmd_parser.h"

int32_t flag_repeats(parsed_cmd_t *cmd, char *flag) {
    for (uint32_t i = 0; i < cmd->args_size; i++) {
        if (strcmp(cmd->args[i].flag, flag) == 0) {
            return 1;
        }
    }

    return 0;
}

int32_t cmd_parse(parsed_cmd_t *cmd, char *buf) {
    memset(cmd, 0, sizeof(parsed_cmd_t));

    uint32_t buf_size = strlen(buf);
    int32_t quote = 0;
    for (uint32_t i = 0; i < buf_size; i++) {
        if (buf[i] == ' ' && !quote) {
            buf[i] = 0;
        }

        if (buf[i] == '"') {
            quote = !quote;
        }
    }

    char *p = buf;

    if (!p) {
        return -1;
    }

    cmd->name = p;

    p = strchr(p, 0);
    int32_t last_tok_flag = 0;

    while (p && p - buf + 1 < buf_size) {
        p = p + 1;

        // `cmd value` case
        if (last_tok_flag == 0 && p[0] != '-') {
            return -1;
        }

        // `cmd -flag` case
        if (last_tok_flag == 0) {
            if (flag_repeats(cmd, p)) {
                return -1;
            }

            cmd->args[cmd->args_size].flag = p;
            last_tok_flag = 1;
        } else {
            // `cmd -flag1 -flag2` case
            if (p[0] == '-') {
                if (flag_repeats(cmd, p)) {
                    return -1;
                }

                cmd->args_size += 1;

                // too many args
                if (cmd->args_size == 10) {
                    return 0;
                }

                cmd->args[cmd->args_size].flag = p;
                last_tok_flag = 1;
            }
            // `cmd -flag value` case
            else {
                // value contains spaces
                if (p[0] == '"') {
                    p += 1;
                    p[strlen(p) - 1] = 0;
                }

                cmd->args[cmd->args_size].value = p;
                last_tok_flag = 0;
                cmd->args_size += 1;

                // too many args
                if (cmd->args_size == 10) {
                    return 0;
                }
            }
        }

        p = strchr(p, 0);

        // skip sequences of NULL
        if (p - buf + 1 < buf_size && strlen(p + 1) == 0) {
            p += 1;
        }
    }

    if (cmd->args[cmd->args_size].flag) {
        cmd->args_size += 1;
    }

    return 0;
}

void print_cmd(log_t log_type, parsed_cmd_t *cmd) {
    print(log_type, "%s: %d args\n", cmd->name, cmd->args_size);

    for (uint32_t i = 0; i < cmd->args_size; i++) {
        if (cmd->args[i].flag) {
            print(log_type, "%s", cmd->args[i].flag);
        }

        if (cmd->args[i].value) {
            print(log_type, " %s", cmd->args[i].value);
        }

        print(log_type, "\n");
    }
}

int32_t is_arg_exclusive(cmd_t *cmd, uint32_t index) {
    for (uint32_t i = 0; cmd->exclusive[i] != -1; i++) {
        if (cmd->exclusive[i] == index) {
            return 1;
        }
    }

    return 0;
}

void print_cmd_arg(log_t log_type, cmd_arg_t *arg) {
    print(log_type,
          "    " ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "," ANSI_COLOR_RED
          "%s" ANSI_COLOR_RESET,
          arg->short_name, arg->long_name);
    if (arg->placeholder) {
        print(log_type, "=" ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET,
              arg->placeholder);
    }
    print(log_type, "\n");

    print(log_type, "        %s\n\n", arg->desc);
}

void print_cmd_help(log_t log_type, cmd_t *cmd) {
    // NAME
    print(log_type,
          ANSI_COLOR_RED "NAME" ANSI_COLOR_RESET "\n"
                         "    %s - %s",
          cmd->cmd_name, cmd->desc);
    print(log_type, "\n\n");

    // SYNOPSIS
    print(log_type,
          ANSI_COLOR_RED "SYNOPSIS" ANSI_COLOR_RESET "\n"
                         "    " ANSI_COLOR_RED "%s" ANSI_COLOR_RESET " ",
          cmd->cmd_name);
    // print exclusive args if they exist
    if (cmd->exclusive[0] != -1) {
        print(log_type, "[");
        for (uint32_t i = 0; cmd->exclusive[i] != -1; i++) {
            print(log_type, "%s", cmd->args[cmd->exclusive[i]].short_name);

            if (cmd->exclusive[i + 1]) {
                print(log_type, "|");
            }
        }
        print(log_type, "] ");
    }
    for (uint32_t i = 0; i < cmd->args_size; i++) {
        // skip exclusive args
        if (is_arg_exclusive(cmd, i)) {
            continue;
        }

        print(log_type, "%s ", cmd->args[i].short_name);
    }
    print(log_type, "\n\n");

    // DESCRIPTION
    print(log_type, ANSI_COLOR_RED "DESCRIPTION" ANSI_COLOR_RESET "\n");
    print(log_type, "     %s\n\n", cmd->desc);
    for (uint32_t i = 0; i < cmd->args_size; i++) {
        print_cmd_arg(log_type, &cmd->args[i]);
    }
    print(log_type, "\n\n");
}
