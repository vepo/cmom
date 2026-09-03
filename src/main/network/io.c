#include <sys/epoll.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#include "core/logger.h"

#include "network/io.h"
#include "network/watcher.h"

#include "scheduler/tasks.h"

/**
 * Internal function. Implements async I/O read-write loop.
 */
void _async_io_read_write_loop(scheduler_connection_task_t *conn)
{
    bool running = true;
    while (running)
    {
        // 1. All read/write are async, so read never blocks.
        //    If not data is available, it should return 0, EWOULDBLOCK or EAGAIN
        ssize_t bytes_read;
        LOG_DEBUG("Reading current buffer \n\"\"\"\n%s\n\"\"\"\n", conn->protocol.buffer);
        while ((bytes_read = read(conn->socket,
                                  &conn->protocol.buffer[conn->protocol.end],
                                  sizeof(conn->protocol.buffer) - conn->protocol.start)) < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                LOG_DEBUG("Connection not ready for read! Leaving context...");
                // 2. Socket is not ready to read. Add the socket to I/O event watcher
                //    and leave the execution process to broker
                event_watch_io_wait_read(&conn->broker->event_watch, conn->socket);
                coroutines_leave_connection(conn);
            }
            else
            {
                // 3. Some other error. Close the socket!
                LOG_ERROR("Error reading socket! %d", errno);
                running = false;
                break;
            }
        }

        conn->protocol.end += bytes_read;

        LOG_DEBUG("Bytes read %d for connection %d", (int)bytes_read, conn->socket);

        if (bytes_read == 0)
        {
            LOG_DEBUG("No byte read! Closing socket!");
            // 4. Connection closed! Close the socket!
            running = false;
        }

        if (running)
        {
            // 5. All read/write are async. The write will not block
            LOG_DEBUG("Data Received: %.*s", (int)bytes_read, conn->protocol.buffer);
            protocols_process(conn->broker->protocol, &conn->protocol);
            size_t bytes_written_committed = 0;
            while (bytes_written_committed < bytes_read)
            {
                ssize_t bytes_written = write(conn->socket, conn->protocol.buffer + bytes_written_committed, bytes_read - bytes_written_committed);
                if (bytes_written < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // 6. Socket is not available for now. Leave the contexts to main broker.
                        //    And wait for read events.
                        LOG_DEBUG("Connection not ready for write! Leaving context...");
                        event_watch_io_wait_write(&conn->broker->event_watch, conn->socket);
                        coroutines_leave_connection(conn);
                    }
                    else
                    {
                        // 7. Some other error. Close the socket!
                        LOG_ERROR("Error writting socket! %d", errno);
                        running = false;
                        break;
                    }
                }
                else
                {
                    // 8. Write was a successful operation. Commit the bytes and loop again to check if any
                    //    bytes were missing to flush.
                    bytes_written_committed += bytes_written;
                }
            }
        }

        // 9. Write done! Wait for read be ready!
        event_watch_io_wait_read(&conn->broker->event_watch, conn->socket);
    }

    // 10. Connection closed. Cleanup event watcher and close socket.
    event_watch_io_unsubscribe(&conn->broker->event_watch, conn->socket);
    close(conn->socket);
    LOG_DEBUG("Connection closed! connection=%d", conn->socket);
    conn->socket = -1; // tell main that this coroutine is done
}

scheduler_connection_task_t *async_io_init(scheduler_broker_task_t *broker, int socket)
{
    return coroutines_schedule(socket, broker, (void *)_async_io_read_write_loop);
}

void network_io_async_enable(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}