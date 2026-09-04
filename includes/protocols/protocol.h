#ifndef PROTOCOLS_PROTOCOL_H
#define PROTOCOLS_PROTOCOL_H

#include <sys/types.h>
#include <stdbool.h>

#define BUFFER_SIZE 4096

typedef enum
{
    STOMP
} protocol_t;

typedef enum
{
    UNKNOWN = -1,
    CONNECT = 1,
    SUBSCRIBE = 2,
    MESSAGE = 3,
    HEARTBEAT = 4
} command_t;

typedef struct protocol_header protocol_header_t;
typedef struct protocol_request protocol_request_t;

typedef struct protocol_header
{
    char *key;
    char *value;
    protocol_header_t *next_header;
} protocol_header_t;

typedef struct protocol_request
{
    command_t command;
    protocol_header_t *headers;
    unsigned char *body;
    size_t body_len;
    size_t body_received;
    bool ready;
    protocol_request_t *next_request;
} protocol_request_t;

/**
 * \brief Protocol Reading Buffer.
 */
typedef struct protocol_buffer
{
    unsigned char input_buffer[BUFFER_SIZE]; /**< I/O buffer for read/write operations. */
    unsigned char *working_buffer;           /**< Auxiliar buffer. Allow storage temporary data until the data is fully ready. */
    int start;                               /** Start of read buffer */
    int end;                                 /** End of read buffer */
    bool corrupted;
    protocol_request_t *requests;
    size_t processing_stage;
} protocol_buffer_t;

typedef struct protocol_response protocol_response_t;
typedef struct protocol_response
{
    unsigned char *content;
    size_t length;
    protocol_response_t *next;
} protocol_response_t;

void protocols_initialize(protocol_t protocol, protocol_buffer_t *buffer);
void protocols_consume_input_buffer(protocol_t protocol, protocol_buffer_t *buffer);
protocol_response_t *protocols_handle_requests(protocol_buffer_t *buffer);
void protocols_response_release(protocol_response_t *resp);

#endif