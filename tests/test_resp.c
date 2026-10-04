#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "resp.h"

int main(void) {
    const char *input = "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n";
    size_t input_len = strlen(input);

    // every strict prefix is valid so far but incomplete
    for (size_t i = 0; i < input_len; i++) {
        resp_command_t partial;
        assert(resp_parse_command(input, i, &partial) == RESP_INCOMPLETE);
    }

    // the full buffer parses once all the bytes are there
    resp_command_t cmd;
    long consumed = resp_parse_command(input, input_len, &cmd);

    assert(consumed == (long)input_len); // the whole buffer was exactly one command
    assert(cmd.argc == 2);
    assert(cmd.argvlen[0] == 3);
    assert(memcmp(cmd.argv[0], "GET", 3) == 0);
    assert(cmd.argvlen[1] == 3);
    assert(memcmp(cmd.argv[1], "foo", 3) == 0);

    resp_command_free(&cmd);

    // two commands back to back, like one read() that got a pipeline
    const char *pipe = "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n*1\r\n$4\r\nPING\r\n";
    size_t pipe_len = strlen(pipe);
    size_t first_len = strlen("*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n");

    resp_command_t first;
    long n1 = resp_parse_command(pipe, pipe_len, &first);
    assert(n1 == (long)first_len); // stopped exactly at the second command
    assert(first.argc == 2);
    assert(memcmp(first.argv[0], "GET", 3) == 0);
    resp_command_free(&first);

    resp_command_t second;
    long n2 = resp_parse_command(pipe + n1, pipe_len - (size_t)n1, &second);
    assert(n2 == (long)(pipe_len - (size_t)n1)); // second command took the rest
    assert(second.argc == 1);
    assert(second.argvlen[0] == 4);
    assert(memcmp(second.argv[0], "PING", 4) == 0);
    resp_command_free(&second);

    // a pipeline whose last command is still arriving
    const char *tail = "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n*1\r\n$4\r\nPI";
    resp_command_t done;
    long t1 = resp_parse_command(tail, strlen(tail), &done);
    assert(t1 == (long)first_len);
    resp_command_free(&done);
    assert(resp_parse_command(tail + t1, strlen(tail) - (size_t)t1, &done) == RESP_INCOMPLETE);

    // malformed input is an error, not just incomplete
    resp_command_t bad;
    assert(resp_parse_command("+OK\r\n", 5, &bad) == RESP_ERROR);
    assert(resp_parse_command("*2\r\n:3\r\n", 8, &bad) == RESP_ERROR);
    assert(resp_parse_command("*x\r\n", 4, &bad) == RESP_ERROR);

    printf("test_resp: all assertions passed\n");
    return 0;
}
