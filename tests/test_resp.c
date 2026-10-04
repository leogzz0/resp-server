#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "resp.h"
#include "sds.h"

// feeds the stream in pieces of 'chunk' bytes and checks every command comes out in order
static void check_stream(const char *stream, size_t chunk) {
    const char *names[] = {"GET", "PING", "SET"};
    const size_t argcs[] = {2, 1, 3};
    size_t stream_len = strlen(stream);

    sds_t buf;
    sds_init(&buf);
    size_t start = 0; // where the next unparsed command begins
    size_t done = 0;  // how many commands were parsed so far

    for (size_t fed = 0; fed < stream_len; fed += chunk) {
        size_t take = stream_len - fed < chunk ? stream_len - fed : chunk;
        sds_append(&buf, stream + fed, take);

        // parse as many complete commands as the buffer holds right now
        for (;;) {
            resp_command_t cmd;
            long n = resp_parse_command(buf.data + start, buf.len - start, &cmd);
            if (n == RESP_INCOMPLETE) {
                break; // wait for the next piece
            }
            assert(n > 0); // a valid stream must never produce an error
            assert(done < 3);
            assert(cmd.argc == argcs[done]);
            assert(cmd.argvlen[0] == strlen(names[done]));
            assert(memcmp(cmd.argv[0], names[done], strlen(names[done])) == 0);
            resp_command_free(&cmd);
            start += (size_t)n;
            done++;
        }
    }

    assert(done == 3); // all three commands came out
    assert(start == stream_len); // nothing left over
    sds_free(&buf);
}

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

    // same stream fed byte by byte, in 3-byte pieces, and all at once
    const char *stream = "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n"
                         "*1\r\n$4\r\nPING\r\n"
                         "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nhello\r\n";
    check_stream(stream, 1);
    check_stream(stream, 3);
    check_stream(stream, strlen(stream));

    printf("test_resp: all assertions passed\n");
    return 0;
}
