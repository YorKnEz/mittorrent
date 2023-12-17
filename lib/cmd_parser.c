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
        while (p - buf + 1 < buf_size && *p == 0) {
            p = p + 1;
        }

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
    }

    if (cmd->args[cmd->args_size].flag) {
        cmd->args_size += 1;
    }

    return 0;
}

void print_parsed_cmd(log_t log_type, parsed_cmd_t *cmd) {
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

    // count exclusive args
    int32_t excl = 0;
    for (uint32_t i = 0; i < cmd->args_size; i++) {
        if (cmd->args[i].excl) {
            excl++;
        }
    }

    // print exclusive args if they exist
    for (uint32_t i = 0, excli = 0; excl && i < cmd->args_size; i++) {
        if (cmd->args[i].excl) {
            if (excli == 0) {
                print(LOG, "[");
            }

            print(LOG, "%s", cmd->args[i].short_name);

            excli++;

            if (excli == excl) {
                print(LOG, "] ");
            } else {
                print(LOG, "|");
            }
        }
    }

    // print the rest of the args
    for (uint32_t i = 0; i < cmd->args_size; i++) {
        if (!cmd->args[i].excl) {
            print(log_type, "[%s] ", cmd->args[i].short_name);
        }
    }
    print(log_type, "\n\n");

    // DESCRIPTION
    print(log_type, ANSI_COLOR_RED "DESCRIPTION" ANSI_COLOR_RESET "\n");
    print(log_type, "     %s\n\n", cmd->desc);
    for (uint32_t i = 0; i < cmd->args_size; i++) {
        print_cmd_arg(log_type, &cmd->args[i]);
    }
}

void cmds_get_cmd_type(cmds_t *cmds, parsed_cmd_t *cmd, cmd_type_t *cmd_type) {
    for (uint32_t i = 0; i < cmds->size; i++) {
        if (strcmp(cmd->name, cmds->list[i].cmd_name) == 0) {
            if (cmd->args_size == 0) {
                *cmd_type = cmds->list[i].cmd_type;
                break;
            }

            // take each arg and validate it
            for (uint32_t j = 0; j < cmd->args_size; j++) {
                uint32_t arg = -1;

                for (uint32_t k = 0; k < cmds->list[i].args_size; k++) {
                    if (strcmp(cmd->args[j].flag,
                               cmds->list[i].args[k].short_name) == 0 ||
                        strcmp(cmd->args[j].flag,
                               cmds->list[i].args[k].long_name) == 0) {
                        arg = k;

                        break;
                    }
                }

                if (-1 == arg) {
                    ERR_GENERIC("invalid flag %s", cmd->args[j].flag);
                    *cmd_type = CMD_ERROR;
                    break;
                }

                if (cmds->list[i].args[arg].excl && cmd->args_size > 1) {
                    ERR_GENERIC("flag %s should be used alone",
                                cmd->args[j].flag);
                    *cmd_type = CMD_ERROR;
                    break;
                }

                // check if value is non null
                if (cmds->list[i].args[arg].placeholder && !cmd->args[j].value) {
                    ERR_GENERIC("value must be non-null for %s",
                                cmd->args[j].flag);
                    *cmd_type = CMD_ERROR;
                    break;
                }

                *cmd_type = cmds->list[i].args[arg].cmd_type;
            }

            break;
        }
    }
}

void print_cmds_help(log_t log_type, cmds_t *cmds) {
    // PROLOG
    print(log_type,
          ANSI_COLOR_RED "PROLOG\n" ANSI_COLOR_RESET
                         "    %s\n\n", cmds->prolog);

    // DESCRIPTION
    print(log_type, ANSI_COLOR_RED "DESCRIPTION\n" ANSI_COLOR_RESET
                              "    %s\n\n", cmds->desc);

    // COMMANDS
    print(log_type, ANSI_COLOR_RED "COMMANDS\n" ANSI_COLOR_RESET);
    
    for (uint32_t i = 0; i < cmds->size; i++) {
        if (cmds->list[i].args_size > 0) {
            print(log_type, "    " ANSI_COLOR_GREEN "%10s" ANSI_COLOR_RESET " - See " ANSI_COLOR_GREEN "%s -h" ANSI_COLOR_RESET ".", cmds->list[i].cmd_name, cmds->list[i].cmd_name);
        } else {
            print(log_type, "    " ANSI_COLOR_GREEN "%10s" ANSI_COLOR_RESET " - %s", cmds->list[i].cmd_name, cmds->list[i].desc);
        }
        print(log_type, "\n\n");
    }
}
