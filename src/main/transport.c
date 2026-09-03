#include "transport.h"

#include <netinet/ip.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "core/logger.h"
#include "async_io.h"

void transport_init(int port, int max_connection_queue, transport_t *transport)
{
    // 1. Creates the Socket and using IPv4 AF_INET and TCP SOCK_STREAM
    // DOC: https://man7.org/linux/man-pages/man2/socket.2.html
    transport->socket = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Set socket options. Using TCP
    // DOC: https://man7.org/linux/man-pages/man2/setsockopt.2.html
    int opt = 1;
    setsockopt(transport->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // 3. Bind the socket
    struct sockaddr_in addr = { .sin_family = AF_INET,        // Use IPv4 https://man7.org/linux/man-pages/man7/address_families.7.html
                                .sin_port = htons(port),      // Listen port
                                .sin_addr.s_addr = INADDR_ANY // Accept from any address.
                                                              // Other options are INADDR_LOOPBACK   127.0.0.1
                                                              //                   INADDR_ANY        0.0.0.0
                                                              //                   INADDR_BROADCAST  255.255.255.255
                              };
    bind(transport->socket, (struct sockaddr *)&addr, sizeof(addr));
    // 4. Set file descriptor to non_blocking
    async_io_enable(transport->socket);
    // 5. Mark the socket as passive accepting at most max_connection_queue connections in queue
    // DOC: https://man7.org/linux/man-pages/man2/listen.2.html
    listen(transport->socket, max_connection_queue);
}

int transport_start_connection(transport_t *transport)
{
    // 1. Accept the new connection
    // DOC: https://man7.org/linux/man-pages/man2/accept.2.html
    int connection = accept(transport->socket, NULL, NULL);
    if (connection < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("accept");
        }
        return -1;
    }
    // 2. Set the new connection as async
    async_io_enable(connection);
    LOG_DEBUG("New connection: %d", connection);
    return connection;
}