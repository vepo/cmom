#include "scheduler/tasks.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "protocols/protocol.h"

scheduler_connection_task_t *coroutines_schedule(int socket, scheduler_broker_task_t *broker, void *process_connection_fn)
{
    scheduler_connection_task_t *conn = malloc(sizeof(scheduler_connection_task_t));
    if (!conn)
    {
        return NULL;
    }

    conn->socket = socket;
    conn->broker = broker;
    protocols_initialize(broker->protocol, &conn->protocol);    

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

void coroutines_join_connection(scheduler_connection_task_t *connection)
{
    swapcontext(&connection->broker->context, &connection->context);
}

void coroutines_leave_connection(scheduler_connection_task_t *connection)
{
    swapcontext(&connection->context, &connection->broker->context);
}
