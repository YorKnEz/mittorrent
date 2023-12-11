#include "cmd_parser.h"

int32_t flag_repeats(cmd_t *cmd, char *flag) {
    for (uint32_t i = 0; i < cmd->args_size; i++) {
        if (strcmp(cmd->args[i].flag, flag) == 0) {
            return 1;
        }
    }

    return 0;
}

int32_t cmd_parse(cmd_t *cmd, char *buf) {
    memset(cmd, 0, sizeof(cmd_t));

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

void print_cmd(log_t log_type, cmd_t *cmd) {
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