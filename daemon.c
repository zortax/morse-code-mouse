#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

int main() {
    int fd;
    int out;
    char *fifoname = "/home/leo/test.fifo";
    char *mouse = "/dev/input/mice";
    fd = open(mouse, O_RDONLY);
    mkfifo(fifoname, 0666);
    out = open(fifoname, O_WRONLY);
    if (fd < 0) {
        printf("error\n");
    }
    unsigned char buf[3];
    while (1) {
        int r = read(fd, buf, 3);
        if (r == 3) {
            write(out, buf, 3);
        }
    }
}
