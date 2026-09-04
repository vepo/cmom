#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "core/logger.h"
#include "core/lifecycle.h"

#include "network/watcher.h"
#include "network/transport.h"
#include "network/io.h"

#include "storage/engine.h"

#include "scheduler/tasks.h"

transport_t transport;

void broker_cleanup(void *args)
{
    LOG_DEBUG("Application shutdown. Closing broker....");
    scheduler_broker_task_t *broker = (scheduler_broker_task_t *)args;
    broker->running = false;
    if (broker->connections)
    {
        for (int conn_idx = 0; conn_idx < MAX_CONNECTIONS; ++conn_idx)
        {
            if (broker->connections[conn_idx] && broker->connections[conn_idx]->socket)
            {
                close(broker->connections[conn_idx]->socket);
                broker->connections[conn_idx]->socket = -1;
            }
        }
    }
    if (transport.socket)
    {
        close(transport.socket);
    }
    LOGGER_CLEANUP();
}

/**
 * @brief Main entry point – runs the event loop.
 *
 * Initialises the transport, epoll, and the main coroutine context.
 * The loop waits for epoll events and dispatches them either to accept
 * new connections or to resume existing coroutines. The loop exits when
 * the `running` flag becomes 0 (set by SIGINT).
 *
 * @return 0 on normal exit.
 */
int main(void)
{
    LOGGER_INIT(); // must be called before any LOG_* macro
    LOG_INFO("Broker starting up...");

    storage_engine_init("./storage");

    // 1. Initialize the transport layer. Create the server and
    //    start listening to new connections
    int listen_port = 8080;
    transport_init(listen_port, 128, &transport);
    scheduler_broker_task_t broker = {.connections = {NULL},
                                      .running = true,
                                      .protocol = STOMP};

    // 2. Initialize event watcher and subscribe for transport layer events
    event_watch_init(&broker.event_watch);
    event_watch_io_subscribe(&broker.event_watch, transport.socket);

    // 3. Setup signal handler to graceful shutdown.
    //    Watch CTRL+C signal
    core_lifecycle_add_shutdown_hookpoint(&broker_cleanup, &broker);

    // 6. Initialize coroutines by storing the current contexts in broker context
    // DOC: https://man7.org/linux/man-pages/man2/getcontext.2.html
    getcontext(&broker.context);

    LOG_DEBUG("Listening on port %d", listen_port);

    // 7. Event loop that looks for I/O events. It should always back to this event loop
    //    if there is read/write block.
    while (broker.running)
    {
        // 8. Block until I/O events found.
        //    Than intract over all events
        int num_events = event_watch_io_wait(&broker.event_watch);
        for (int evt_idx = 0; evt_idx < num_events; ++evt_idx)
        {
            // 9. Get the event file descriptor.
            int event_fd = broker.event_watch.events[evt_idx].data.fd;

            // 10. Compare with transport layer socket.
            if (event_fd == transport.socket)
            {
                // 11. If there is an event on transport layer, this means a new connection was open.
                //     Start this connection.
                int connection = transport_start_connection(&transport);
                if (connection)
                {
                    // 11. Initialize the I/O processor
                    scheduler_connection_task_t *conn = async_io_init(&broker, connection);
                    if (!conn)
                    {
                        close(connection);
                        continue;
                    }
                    // 12. Connection is open and the I/O process is configured.
                    //     Subscribe for events on open connection
                    event_watch_io_subscribe(&broker.event_watch, connection);
                }
            }
            else
            {
                // 13. The event is not on transport layer. Maybe in open connection.
                scheduler_connection_task_t *conn = broker.connections[event_fd];
                if (!conn || conn->socket == -1)
                {
                    // 14. Stale event! Close connection!
                    event_watch_io_unsubscribe(&broker.event_watch, event_fd);
                    close(event_fd);
                    broker.connections[event_fd] = NULL;
                    continue;
                }

                // 14. Join the connection coroutine
                coroutines_join_connection(conn);

                // 15. Coroutine is done! This means the connection was closed.
                //     Release main structures
                if (conn->socket == -1)
                {
                    // Coroutine has cleaned itself up – free resources
                    free(conn->context.uc_stack.ss_sp);
                    free(conn);
                    broker.connections[event_fd] = NULL;
                    LOG_DEBUG("Connection %d removed", event_fd);
                }
            }
        }
    }

    return 0;
}