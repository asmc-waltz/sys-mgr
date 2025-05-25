#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/eventfd.h>

#include <comm.h>

int event_fd;

void event_set(int evfd, uint64_t code)
{
    uint64_t val = code;
    write(evfd, &val, sizeof(val));
}

uint64_t event_get(int evfd)
{
    uint64_t val;
    read(evfd, &val, sizeof(uint64_t));
    return val;
}

int init_event_file()
{
    event_fd = eventfd(0, EFD_NONBLOCK);
    if (event_fd == -1) {
        perror("Init eventfd error");
        return -1;
    }
}
