#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    printf("resp-server: socket opened (fd=%d), closing...\n", fd);

    close(fd);
    return 0;
}
