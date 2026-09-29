#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "resp.h"

int main(void) {
    const char *input = "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n";
    size_t input_len = strlen(input);

    resp_command_t cmd;
    long consumed = resp_parse_command(input, input_len, &cmd);

    assert(consumed == (long)input_len); // the whole buffer was exactly one command
    assert(cmd.argc == 2);
    assert(cmd.argvlen[0] == 3);
    assert(memcmp(cmd.argv[0], "GET", 3) == 0);
    assert(cmd.argvlen[1] == 3);
    assert(memcmp(cmd.argv[1], "foo", 3) == 0);

    resp_command_free(&cmd);

    printf("test_resp: all assertions passed\n");
    return 0;
}
