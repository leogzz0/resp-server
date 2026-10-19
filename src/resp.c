#include "resp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// finds the next \r\n at or after 'from', returns the index of '\r' or -1
static long find_crlf(const char *buf, size_t len, size_t from) {
    for (size_t i = from; i + 1 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') {
            return (long)i;
        }
    }
    return -1;
}

// parses a decimal integer from buf[start, end), or -1 if it's not all digits
static long parse_int(const char *buf, size_t start, size_t end) {
    if (start >= end) {
        return -1;
    }
    long value = 0;
    for (size_t i = start; i < end; i++) {
        if (buf[i] < '0' || buf[i] > '9') {
            return -1;
        }
        value = value * 10 + (buf[i] - '0');
    }
    return value;
}

long resp_parse_command(const char *buf, size_t len, resp_command_t *cmd) {
    size_t pos = 0;

    if (len == 0) {
        return RESP_INCOMPLETE; // nothing received yet
    }
    if (buf[pos] != '*') {
        return RESP_ERROR; // every command starts with an array marker
    }
    pos++;

    long line_end = find_crlf(buf, len, pos);
    if (line_end < 0) {
        return RESP_INCOMPLETE; // array header not fully received
    }
    long argc = parse_int(buf, pos, (size_t)line_end);
    if (argc <= 0) {
        return RESP_ERROR;
    }
    pos = (size_t)line_end + 2; // skip the \r\n

    char **argv = malloc(sizeof(char *) * (size_t)argc);
    size_t *argvlen = malloc(sizeof(size_t) * (size_t)argc);
    size_t filled = 0; // how many argv slots are actually allocated so far
    long ret = RESP_ERROR;

    for (long i = 0; i < argc; i++) {
        if (pos >= len) {
            ret = RESP_INCOMPLETE;
            goto cleanup;
        }
        if (buf[pos] != '$') {
            goto cleanup; // expected a bulk string marker
        }
        pos++;

        long bulk_end = find_crlf(buf, len, pos);
        if (bulk_end < 0) {
            ret = RESP_INCOMPLETE;
            goto cleanup;
        }
        long bulk_len = parse_int(buf, pos, (size_t)bulk_end);
        if (bulk_len < 0) {
            goto cleanup;
        }
        pos = (size_t)bulk_end + 2; // skip the \r\n after the length

        if (pos + (size_t)bulk_len + 2 > len) {
            ret = RESP_INCOMPLETE; // payload or its trailing crlf still on the way
            goto cleanup;
        }
        if (buf[pos + (size_t)bulk_len] != '\r' || buf[pos + (size_t)bulk_len + 1] != '\n') {
            goto cleanup;
        }

        argv[i] = malloc((size_t)bulk_len + 1);
        memcpy(argv[i], buf + pos, (size_t)bulk_len);
        argv[i][bulk_len] = '\0'; // handy for printing and strcmp
        argvlen[i] = (size_t)bulk_len;
        filled++;

        pos += (size_t)bulk_len + 2; // skip the payload and its trailing crlf
    }

    cmd->argv = argv;
    cmd->argvlen = argvlen;
    cmd->argc = (size_t)argc;
    return (long)pos;

cleanup:
    for (size_t i = 0; i < filled; i++) {
        free(argv[i]); // only free the slots we actually allocated
    }
    free(argv);
    free(argvlen);
    return ret;
}

void resp_command_free(resp_command_t *cmd) {
    for (size_t i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }
    free(cmd->argv);
    free(cmd->argvlen);
    cmd->argv = NULL;
    cmd->argvlen = NULL;
    cmd->argc = 0;
}

void resp_add_simple(sds_t *out, const char *s) {
    sds_append(out, "+", 1);
    sds_append(out, s, strlen(s));
    sds_append(out, "\r\n", 2);
}

void resp_add_error(sds_t *out, const char *msg) {
    sds_append(out, "-", 1);
    for (const char *p = msg; *p != '\0'; p++) {
        char c = (*p == '\r' || *p == '\n') ? ' ' : *p; // a newline would end the reply early
        sds_append(out, &c, 1);
    }
    sds_append(out, "\r\n", 2);
}

// writes a prefix char followed by a number and \r\n, shared by :, $ and *
static void add_prefixed_number(sds_t *out, char prefix, long long n) {
    char tmp[32];
    int len = snprintf(tmp, sizeof(tmp), "%c%lld\r\n", prefix, n);
    sds_append(out, tmp, (size_t)len);
}

void resp_add_integer(sds_t *out, long long n) {
    add_prefixed_number(out, ':', n);
}

void resp_add_bulk(sds_t *out, const char *data, size_t len) {
    add_prefixed_number(out, '$', (long long)len); // length header
    sds_append(out, data, len); // payload, may contain any bytes
    sds_append(out, "\r\n", 2);
}

void resp_add_nil(sds_t *out) {
    sds_append(out, "$-1\r\n", 5);
}

void resp_add_array_header(sds_t *out, size_t count) {
    add_prefixed_number(out, '*', (long long)count);
}
