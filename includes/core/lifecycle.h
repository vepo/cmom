#ifndef CORE_LIFECYCLE_H
#define CORE_LIFECYCLE_H

typedef void (*hookpoint_fn)(void * args);

void core_lifecycle_add_shutdown_hookpoint(hookpoint_fn shutdown_fn, void * shutdown_args);

#endif /* CORE_LIFECYCLE_H */