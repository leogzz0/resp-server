#ifndef CMD_H
#define CMD_H

#include "resp.h"
#include "sds.h"
#include "server.h"

// a handler appends its reply to out
typedef void (*cmd_handler_t)(server_t *srv, const resp_command_t *cmd, sds_t *out);

typedef struct {
    const char *name;     // upper case, matched case-insensitively
    cmd_handler_t handler;
    int min_argc;         // counts the command name itself
    int max_argc;         // -1 means no upper limit
} cmd_def_t;

// finds the command, checks its arg count, runs it, and appends the reply to out
void cmd_execute(server_t *srv, const resp_command_t *cmd, sds_t *out);

#endif
