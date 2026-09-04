#include "core/lifecycle.h"

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "core/logger.h"

typedef struct core_hookpoint core_hookpoint_t;

typedef struct core_hookpoint
{
    hookpoint_fn fn;
    void *args;
    volatile core_hookpoint_t *next_function;
} core_hookpoint_t;

volatile core_hookpoint_t *_core_shutdown_hookpoints = NULL;

void _core_lifecycle_shutdown_impl()
{
    volatile core_hookpoint_t *current_hookpoint = _core_shutdown_hookpoints;
    while (current_hookpoint != NULL)
    {
        current_hookpoint->fn(current_hookpoint->args);
        current_hookpoint = current_hookpoint->next_function;
    };
}

void _core_lifecycle_atexit_callback(void)
{
    LOG_INFO("Broker exit! Calling shutdown hookpoints...");
    _core_lifecycle_shutdown_impl();
}

void _core_lifecycle_signal_callback(int signal)
{
    LOG_INFO("Interrupt signal received! Calling shutdown hookpoints... singal=%d", signal);
    _core_lifecycle_shutdown_impl();
    _exit(0); // avoid atexit handlers
}

void core_lifecycle_add_shutdown_hookpoint(hookpoint_fn shutdown_fn, void *shutdown_args)
{
    if (_core_shutdown_hookpoints == NULL)
    {
        atexit(_core_lifecycle_atexit_callback);
        signal(SIGINT, _core_lifecycle_signal_callback);
        signal(SIGTERM, _core_lifecycle_signal_callback);
    }
    core_hookpoint_t *hookpoint = malloc(sizeof(core_hookpoint_t));
    hookpoint->fn = shutdown_fn;
    hookpoint->args = shutdown_args;

    if (_core_shutdown_hookpoints != NULL)
    {
        hookpoint->next_function = _core_shutdown_hookpoints;
    }
    else
    {
        hookpoint->next_function = NULL;
    }
    _core_shutdown_hookpoints = hookpoint;
}