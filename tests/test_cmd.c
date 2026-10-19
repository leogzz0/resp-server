#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cmd.h"

static server_t srv; // shared on purpose, commands build on earlier state

// parses a resp command, runs it, and checks the exact reply bytes
static void run(const char *input, const char *expected) {
    resp_command_t cmd;
    long n = resp_parse_command(input, strlen(input), &cmd);
    assert(n == (long)strlen(input));

    sds_t out;
    sds_init(&out);
    cmd_execute(&srv, &cmd, &out);
    assert(out.len == strlen(expected));
    assert(memcmp(out.data, expected, out.len) == 0);

    sds_free(&out);
    resp_command_free(&cmd);
}

int main(void) {
    server_init(&srv);

    run("*1\r\n$4\r\nPING\r\n", "+PONG\r\n");
    run("*1\r\n$4\r\nping\r\n", "+PONG\r\n"); // names are case-insensitive
    run("*1\r\n$4\r\nPiNg\r\n", "+PONG\r\n");
    run("*2\r\n$4\r\nPING\r\n$2\r\nhi\r\n", "$2\r\nhi\r\n");

    run("*2\r\n$4\r\nECHO\r\n$5\r\nhello\r\n", "$5\r\nhello\r\n");

    // arity checks
    run("*3\r\n$4\r\nPING\r\n$1\r\na\r\n$1\r\nb\r\n",
        "-ERR wrong number of arguments for 'ping' command\r\n");
    run("*1\r\n$4\r\nECHO\r\n", "-ERR wrong number of arguments for 'echo' command\r\n");
    run("*3\r\n$4\r\nECHO\r\n$1\r\na\r\n$1\r\nb\r\n",
        "-ERR wrong number of arguments for 'echo' command\r\n");

    // unknown commands
    run("*1\r\n$3\r\nFOO\r\n", "-ERR unknown command 'FOO'\r\n");
    run("*1\r\n$4\r\na\r\nb\r\n", "-ERR unknown command 'a  b'\r\n"); // crlf in a name can't inject lines

    // get on a missing key
    run("*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n", "$-1\r\n");

    // set then get
    run("*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n", "+OK\r\n");
    run("*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n", "$3\r\nbar\r\n");

    // set overwrites
    run("*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbaz\r\n", "+OK\r\n");
    run("*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n", "$3\r\nbaz\r\n");

    // an empty value is a real value, not nil
    run("*3\r\n$3\r\nSET\r\n$5\r\nempty\r\n$0\r\n\r\n", "+OK\r\n");
    run("*2\r\n$3\r\nGET\r\n$5\r\nempty\r\n", "$0\r\n\r\n");

    // exists counts keys, repeated keys count each time
    run("*2\r\n$6\r\nEXISTS\r\n$3\r\nfoo\r\n", ":1\r\n");
    run("*2\r\n$6\r\nEXISTS\r\n$4\r\nnope\r\n", ":0\r\n");
    run("*4\r\n$6\r\nEXISTS\r\n$3\r\nfoo\r\n$3\r\nfoo\r\n$4\r\nnope\r\n", ":2\r\n");
    run("*2\r\n$6\r\nEXISTS\r\n$5\r\nempty\r\n", ":1\r\n");

    // del returns how many keys it removed
    run("*2\r\n$3\r\nDEL\r\n$3\r\nfoo\r\n", ":1\r\n");
    run("*2\r\n$3\r\nDEL\r\n$3\r\nfoo\r\n", ":0\r\n"); // already gone
    run("*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n", "$-1\r\n");
    run("*3\r\n$3\r\nSET\r\n$1\r\na\r\n$1\r\n1\r\n", "+OK\r\n");
    run("*3\r\n$3\r\nSET\r\n$1\r\nb\r\n$1\r\n2\r\n", "+OK\r\n");
    run("*4\r\n$3\r\nDEL\r\n$1\r\na\r\n$1\r\nb\r\n$1\r\nc\r\n", ":2\r\n"); // c never existed

    // arity checks for the new commands
    run("*1\r\n$3\r\nGET\r\n", "-ERR wrong number of arguments for 'get' command\r\n");
    run("*2\r\n$3\r\nSET\r\n$1\r\nk\r\n", "-ERR wrong number of arguments for 'set' command\r\n");
    run("*4\r\n$3\r\nSET\r\n$1\r\nk\r\n$1\r\nv\r\n$1\r\nx\r\n",
        "-ERR wrong number of arguments for 'set' command\r\n");
    run("*1\r\n$3\r\nDEL\r\n", "-ERR wrong number of arguments for 'del' command\r\n");
    run("*1\r\n$6\r\nEXISTS\r\n", "-ERR wrong number of arguments for 'exists' command\r\n");

    server_free(&srv); // asan checks every stored value gets released

    printf("test_cmd: all assertions passed\n");
    return 0;
}
