#include "protocols/protocol.h"

#include "core/logger.h"
#include "protocols/stomp.h"

void protocols_initialize(protocol_t protocol, protocol_buffer_t *buffer)
{
    buffer->requests = NULL;
    buffer->input_buffer[0] = '\0';
    buffer->working_buffer = NULL;
    buffer->start = 0;
    buffer->end = 0;
    buffer->corrupted = false;
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

protocol_response_t * protocols_handle_requests(protocol_buffer_t * buffer)
{
    protocol_response_t *response = NULL;
    if (buffer->requests != NULL)
    {
        protocol_request_t *current_request = buffer->requests;
        while (current_request->ready)
        {
            current_request = current_request->next_request;
        }
    }
    return response;
}

void protocols_consume_input_buffer(protocol_t protocol, protocol_buffer_t *buffer)
{
    LOG_DEBUG("Processing buffer \n\"\"\"\n%s\n\"\"\"\n", buffer->input_buffer);
    switch (protocol)
    {
    case STOMP:
        protocols_stomp_process(buffer);
        break;
    default:
        LOG_FATAL("Protocol not supported! protocol=%d", protocol);
        exit(-1);
    }
}

void protocols_response_release(protocol_response_t *resp)
{
    free(resp->content);
}