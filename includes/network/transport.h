#ifndef NETWORK_TRANSPORT_H
#define NETWORK_TRANSPORT_H

/**
 * @brief Transport layer context holding the listening socket.
 */
typedef struct transport {
    int socket; /**< Listening socket file descriptor. */
} transport_t;

/**
 * @brief Initialise the TCP transport layer.
 *
 * Creates a non‑blocking IPv4 TCP socket, sets SO_REUSEADDR, binds to the
 * specified port on all interfaces, and starts listening with a given backlog.
 *
 * @param port                   Port number to listen on.
 * @param max_connection_queue   Maximum length of the pending connection queue (backlog).
 * @param transport              Pointer to the transport_t structure to initialise.
 */
void transport_init(int port, int max_connection_queue, transport_t *transport);

/**
 * @brief Accept an incoming client connection.
 *
 * Calls accept(2) on the listening socket. The new socket is automatically
 * set to non‑blocking mode via async_io_enable().
 *
 * @param transport  The transport context.
 *
 * @return New client socket fd on success, or -1 if accept fails (including EAGAIN/EWOULDBLOCK).
 */
int transport_start_connection(transport_t *transport);

#endif /* NETWORK_TRANSPORT_H */