#ifndef NETWORK_WATCHER_H
#define NETWORK_WATCHER_H

#include <sys/epoll.h>

#define EVENT_WATCH_MAX 1024

/**
 * @brief Event Watcher context.
 */
typedef struct event_watch {
    int io_poll;                                /**< epoll file descriptor. */
    struct epoll_event events[EVENT_WATCH_MAX]; /**< Array to store returned events. */
} event_watch_t;

/**
 * @brief Initialise the epoll instance.
 *
 * Creates an epoll file descriptor using epoll_create1(0).
 *
 * @param event_watch The event_watch_t structure to initialise.
 */
void event_watch_init(event_watch_t *event_watch);

/**
 * @brief Subscribe a file descriptor for read events (EPOLLIN).
 *
 * Adds the fd to the epoll interest list with EPOLLIN flag.
 *
 * @param event_watch The event watch context.
 * @param fd          File descriptor to watch for readability.
 */
void event_watch_io_subscribe(event_watch_t *event_watch, int fd);

/**
 * @brief Unsubscribe a file descriptor from epoll.
 *
 * Removes the fd from the epoll interest list.
 *
 * @param event_watch The event watch context.
 * @param fd          File descriptor to remove.
 */
void event_watch_io_unsubscribe(event_watch_t *event_watch, int fd);

/**
 * @brief Wait for I/O events with a 100 ms timeout.
 *
 * Calls epoll_wait(2) and stores ready events in event_watch->events.
 * The timeout is set to 100 ms to allow the event loop to check a termination flag.
 *
 * @param event_watch The event watch context.
 *
 * @return Number of ready file descriptors, or -1 on error.
 */
int event_watch_io_wait(event_watch_t *event_watch);

/**
 * @brief Modify a socket to wait for readability (EPOLLIN).
 *
 * Uses EPOLL_CTL_MOD to change the event mask to EPOLLIN. Typically called
 * by a coroutine before yielding because a read would block.
 *
 * @param event_watch The event watch context.
 * @param socket      The socket fd to modify.
 *
 * @return 0 on success, -1 on error (epoll_ctl failure).
 */
int event_watch_io_wait_read(event_watch_t *event_watch, int socket);

/**
 * @brief Modify a socket to wait for writability (EPOLLOUT).
 *
 * Uses EPOLL_CTL_MOD to change the event mask to EPOLLOUT. Called by a coroutine
 * before yielding because a write would block.
 *
 * @param event_watch The event watch context.
 * @param socket      The socket fd to modify.
 *
 * @return 0 on success, -1 on error.
 */
int event_watch_io_wait_write(event_watch_t *event_watch, int socket);

#endif /* NETWORK_WATCHER_H */