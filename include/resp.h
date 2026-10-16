#ifndef RESP_H
#define RESP_H

#include <stddef.h>

#include "sds.h"

#define RESP_ERROR -1      // malformed input
#define RESP_INCOMPLETE -2 // valid so far, need more bytes

// one parsed command, e.g. ["SET", "foo", "bar"]
typedef struct {
    char **argv;      // owned copy of each argument, null-terminated for convenience
    size_t *argvlen;  // length of each argument (args can hold arbitrary bytes)
    size_t argc;      // number of arguments
} resp_command_t;

// parses one resp array command starting at buf[0]
// returns bytes consumed on success, RESP_INCOMPLETE or RESP_ERROR otherwise
// cmd is only filled on success
long resp_parse_command(const char *buf, size_t len, resp_command_t *cmd);

void resp_command_free(resp_command_t *cmd); // frees everything owned by cmd

// reply builders, each appends one encoded value to out
void resp_add_simple(sds_t *out, const char *s);                // +OK\r\n
void resp_add_error(sds_t *out, const char *msg);               // -ERR msg\r\n
void resp_add_integer(sds_t *out, long long n);                 // :42\r\n
void resp_add_bulk(sds_t *out, const char *data, size_t len);   // $3\r\nfoo\r\n
void resp_add_nil(sds_t *out);                                  // $-1\r\n
void resp_add_array_header(sds_t *out, size_t count);           // *2\r\n, then add count elements

#endif
