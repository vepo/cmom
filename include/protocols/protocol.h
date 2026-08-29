#ifndef PROTOCOLS_PROTOCOL_H
#define PROTOCOLS_PROTOCOL_H

#include <sys/types.h>
#include <stdbool.h>

#define BUFFER_SIZE 4096

typedef enum protocol
{
    STOMP
} protocol_e;

typedef struct protocol_header protocol_header_t;
typedef struct protocol_message protocol_message_t;

typedef struct protocol_header
{
    char *key;
    char *value;
    protocol_header_t *next_header;
} protocol_header_t;

typedef struct protocol_message
{
    char command[64];
    protocol_header_t *headers;
    char *body;           // dynamically allocated
    size_t body_len;      // total length (not including null terminator)
    size_t body_received; // how many bytes have been read so far
    bool ready;
    protocol_message_t *next_message;
} protocol_message_t;

typedef struct protocol_buffer
{
    char buffer[BUFFER_SIZE]; /**< I/O buffer for read/write operations. */
    int start;                /** Start of read buffer */
    int end;                  /** End of read buffer */
    protocol_message_t *messages;
    size_t processing_stage;
} protocol_buffer_t;

void protocols_initialize(protocol_e protocol, protocol_buffer_t *buffer);
bool protocols_process(protocol_e protocol, protocol_buffer_t *buffer);

#endif