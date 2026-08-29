#ifndef PROTOCOLS_STOMP_H
#define PROTOCOLS_STOMP_H

#include "protocol.h"

// To be used in processing_state
#define STOMP_PROCESSING_STATE_COMMAND 0
#define STOMP_PROCESSING_STATE_HEADERS 1
#define STOMP_PROCESSING_STATE_BODY    2

bool protocols_stomp_process(protocol_buffer_t * buffer);
void protocols_stomp_initialize(protocol_buffer_t * buffer);

#endif