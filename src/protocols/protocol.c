#include "protocols/protocol.h"

#include "logger.h"
#include "protocols/stomp.h"

void protocols_initialize(protocol_e protocol, protocol_buffer_t *buffer)
{
    buffer->messages = NULL;
    buffer->buffer[0] = '\0';
    buffer->start = 0;
    buffer->end = 0;
    switch (protocol)
    {
    case STOMP:
        protocols_stomp_initialize(buffer);
        break;

    default:
        LOG_FATAL("Protocol not supported! protocol=%d", protocol);
        exit(-1);
    }
}

bool protocols_process(protocol_e protocol, protocol_buffer_t *buffer)
{
    LOG_DEBUG("Processing buffer \n\"\"\"\n%s\n\"\"\"\n", buffer->buffer);
    switch (protocol)
    {
    case STOMP:
        return protocols_stomp_process(buffer);
        break;
    default:
        LOG_FATAL("Protocol not supported! protocol=%d", protocol);
        exit(-1);
    }
    return true;
}