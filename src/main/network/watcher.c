#include "network/watcher.h"

#include <stdlib.h>

void event_watch_init(event_watch_t *event_watch)
{
    // 1. Start epoll. The epoll is a kernel data structure to map a set of file descriptors (fd) to wacth.
    // The epoll contains internally two lists. The interest are the fd that the process has registered to watch
    // and the ready are the set of fd ready to be ready.
    // epoll_create1 returns a fd too.
    // DOC: https://man7.org/linux/man-pages/man7/epoll.7.html
    //      https://man7.org/linux/man-pages/man2/epoll_create.2.html
    //      https://man7.org/linux/man-pages/man2/epoll_ctl.2.html
    //      https://man7.org/linux/man-pages/man2/epoll_wait.2.html
    event_watch->io_poll = epoll_create1(0);
}

void event_watch_io_subscribe(event_watch_t *event_watch, int fd)
{
    struct epoll_event evt = {.events = EPOLLIN,
                              .data.fd = fd};
    epoll_ctl(event_watch->io_poll, EPOLL_CTL_ADD, fd, &evt);
}

void event_watch_io_unsubscribe(event_watch_t *event_watch, int fd)
{
    epoll_ctl(event_watch->io_poll, EPOLL_CTL_DEL, fd, NULL);
}

int event_watch_io_wait(event_watch_t *event_watch)
{
    return epoll_wait(event_watch->io_poll, event_watch->events, EVENT_WATCH_MAX, 100);
}

int event_watch_io_wait_read(event_watch_t *event_watch, int socket)
{
    struct epoll_event evt = {.events = EPOLLIN,
                              .data.fd = socket};
    epoll_ctl(event_watch->io_poll, EPOLL_CTL_MOD, socket, &evt);
}

int event_watch_io_wait_write(event_watch_t *event_watch, int socket)
{
       struct epoll_event evt = {.events = EPOLLOUT,
                              .data.fd = socket};
    epoll_ctl(event_watch->io_poll, EPOLL_CTL_MOD, socket, &evt);
}