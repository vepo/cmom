#ifndef ASYNC_IO_H
#define ASYNC_IO_H

#include "coroutines.h"

/**
 * @brief Enable non‑blocking I/O on a file descriptor.
 *
 * Sets the O_NONBLOCK flag using fcntl(2). This is required for all sockets
 * used with the asynchronous I/O coroutine system.
 *
 * @param fd File descriptor to make non‑blocking.
 */
void async_io_enable(int fd);

/**
 * @brief Spawn a new coroutine for asynchronous I/O on a client socket.
 *
 * Creates a connection context, allocates a stack, and schedules the internal
 * read‑write loop `_async_io_read_write_loop` as a coroutine. The new coroutine
 * will be executed when the main event loop resumes it via `coroutines_join_connection()`.
 *
 * @param broker The broker context (holds the main context and connection table).
 * @param socket The client socket (already accepted and made non‑blocking).
 *
 * @return Pointer to the newly created connection_context_t, or NULL on allocation failure.
 */
connection_context_t *async_io_init(broker_context_t* broker, int socket);

#endif