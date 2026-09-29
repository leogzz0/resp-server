#ifndef RESP_H
#define RESP_H

#include <stddef.h>

// one parsed command, e.g. ["SET", "foo", "bar"]
typedef struct {
    char **argv;      // owned copy of each argument, null-terminated for convenience
    size_t *argvlen;  // length of each argument (args can hold arbitrary bytes)
    size_t argc;      // number of arguments
} resp_command_t;

// parses one resp array command starting at buf[0], assuming it is already complete
// returns bytes consumed on success, -1 on error, and fills cmd on success
long resp_parse_command(const char *buf, size_t len, resp_command_t *cmd);

void resp_command_free(resp_command_t *cmd); // frees everything owned by cmd

#endif
