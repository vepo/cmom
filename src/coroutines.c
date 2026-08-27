#include "coroutines.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

connection_context_t *coroutines_schedule(int socket, broker_context_t *broker, void *process_connection_fn)
{
    connection_context_t *conn = malloc(sizeof(connection_context_t));
    if (!conn)
    {
        return NULL;
    }

    conn->socket = socket;
    conn->broker = broker;

    char *stack = malloc(STACK_SIZE);
    if (!stack)
    {
        free(conn);
        return NULL;
    }

    getcontext(&conn->context);
    conn->context.uc_stack.ss_sp = stack;
    conn->context.uc_stack.ss_size = STACK_SIZE;
    conn->context.uc_link = &broker->context; // automatically return to main when function exits
    makecontext(&conn->context, process_connection_fn, 1, conn);
    broker->connections[socket] = conn;
    return conn;
}

void coroutines_join_connection(connection_context_t *connection)
{
    swapcontext(&connection->broker->context, &connection->context);
}

void coroutines_leave_connection(connection_context_t *connection)
{
    swapcontext(&connection->context, &connection->broker->context);
}
