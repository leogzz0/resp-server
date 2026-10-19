#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "cmd.h"
#include "resp.h"
#include "sds.h"
#include "server.h"

#define PORT 6379

// writes all of data to fd, retrying on partial writes, returns -1 on error
static int write_all(int fd, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, data + sent, len - sent);
        if (n < 0) {
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

// serves one client until it disconnects or sends garbage
static void handle_client(server_t *srv, int client_fd) {
    sds_t in; // bytes received but not parsed yet
    sds_init(&in);
    char chunk[1024];
    ssize_t n;

    while ((n = read(client_fd, chunk, sizeof(chunk))) > 0) {
        sds_append(&in, chunk, (size_t)n);

        // parse every complete command currently in the buffer (pipelining)
        for (;;) {
            resp_command_t cmd;
            long used = resp_parse_command(in.data, in.len, &cmd);
            if (used == RESP_INCOMPLETE) {
                break; // wait for more bytes
            }

            sds_t out; // reply for this command
            sds_init(&out);

            if (used == RESP_ERROR) {
                resp_add_error(&out, "ERR protocol error");
                write_all(client_fd, out.data, out.len);
                sds_free(&out);
                sds_free(&in);
                return; // we can't resync after bad input, drop the client
            }

            cmd_execute(srv, &cmd, &out); // run the command, reply lands in out
            int rc = write_all(client_fd, out.data, out.len);

            sds_free(&out);
            resp_command_free(&cmd);
            sds_consume(&in, (size_t)used); // this command is done, drop it
            if (rc < 0) {
                sds_free(&in);
                return; // client went away mid-write
            }
        }
    }

    if (n < 0) {
        perror("read");
    }
    sds_free(&in);
}

int main(void) {
    server_t srv; // the database lives here and outlives every client
    server_init(&srv);

    // create a tcp socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    // let us rebind to the port right away after restarting
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        return 1;
    }

    // build the address we want to bind to
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // listen on all local interfaces
    addr.sin_port = htons(PORT); // host to network byte order

    // attach the socket to the port
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    // start queueing incoming connections
    if (listen(listen_fd, 16) < 0) {
        perror("listen");
        return 1;
    }

    printf("resp-server: listening on port %d\n", PORT);

    // accept clients forever, one at a time
    for (;;) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("resp-server: client connected (fd=%d)\n", client_fd);

        handle_client(&srv, client_fd);

        printf("resp-server: client disconnected\n");

        close(client_fd); // done with this client, go accept the next one
    }
}
