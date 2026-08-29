#ifndef COROUTINES_H
#define COROUTINES_H

#include <ucontext.h>
#include "event_watch.h"
#include "protocols/protocol.h"

#define STACK_SIZE      (64 * 1024)
#define MAX_CONNECTIONS 4096

typedef struct broker_context broker_context_t;

/**
 * @brief Represents a single client connection with its coroutine context.
 */
typedef struct connection_context {
    ucontext_t context;         /**< Coroutine execution context. */
    broker_context_t *broker;   /**< Pointer to the broker that owns this connection. */
    int socket;                 /**< Client socket file descriptor. */
    protocol_buffer_t protocol;
} connection_context_t;

/**
 * @brief Broker context holding the main coroutine context and all connections.
 */
typedef struct broker_context {
    ucontext_t context;                                 /**< Main coroutine context (event loop). */
    connection_context_t *connections[MAX_CONNECTIONS]; /**< Map fd -> connection_context. */
    event_watch_t event_watch;                          /**< epoll instance and event array. */
    protocol_e protocol;
} broker_context_t;

/**
 * @brief Schedule a coroutine for a new connection.
 *
 * Allocates and initialises a connection_context_t, sets up the coroutine stack,
 * and stores it in the broker's connection table.
 *
 * @param socket                 Client socket fd.
 * @param broker                 The broker context.
 * @param process_connection_fn  Function that will run as the coroutine (takes a `connection_context_t*`).
 *
 * @return Pointer to the new connection context, or NULL on failure.
 */
connection_context_t *coroutines_schedule(int socket, broker_context_t *broker,
                                          void *process_connection_fn);

/**
 * @brief Switch from the main loop to a connection's coroutine.
 *
 * Saves the current (broker) context and restores the coroutine's context.
 * The coroutine will run until it voluntarily yields (calls `coroutines_leave_connection`)
 * or finishes.
 *
 * @param connection The connection whose coroutine should be resumed.
 */
void coroutines_join_connection(connection_context_t *connection);

/**
 * @brief Yield from a coroutine back to the main broker loop.
 *
 * Saves the coroutine's context and restores the broker's context. Called when
 * the coroutine needs to wait for I/O (read/write would block).
 *
 * @param connection The connection that is yielding.
 */
void coroutines_leave_connection(connection_context_t *connection);

#endif