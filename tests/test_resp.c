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

    // malformed input is an error, not just incomplete
    resp_command_t bad;
    assert(resp_parse_command("+OK\r\n", 5, &bad) == RESP_ERROR);
    assert(resp_parse_command("*2\r\n:3\r\n", 8, &bad) == RESP_ERROR);
    assert(resp_parse_command("*x\r\n", 4, &bad) == RESP_ERROR);

    printf("test_resp: all assertions passed\n");
    return 0;
}
