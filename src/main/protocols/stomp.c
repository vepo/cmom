#include "protocols/stomp.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

protocol_message_t *protocols_stomp_message_initialize()
{
    protocol_message_t *message = malloc(sizeof(protocol_message_t));
    message->command[0] = '\0';
    message->next_message = NULL;
    message->headers = NULL;
    message->ready = false;
    return message;
}

// Helper to strip trailing '\r' from a string
static void strip_cr(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\r')
    {
        str[len - 1] = '\0';
    }
}

// Helper to trim leading and trailing spaces (in-place)
static void trim_whitespace(char *str)
{
    // Trim leading
    while (*str == ' ' || *str == '\t')
        str++;
    // Trim trailing
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t'))
    {
        str[len - 1] = '\0';
        len--;
    }
}

// Add a header to the message's header list
static void add_header(protocol_message_t *msg, const char *key, const char *value)
{
    protocol_header_t *new_hdr = malloc(sizeof(protocol_header_t));
    new_hdr->key = strdup(key);
    new_hdr->value = strdup(value);
    new_hdr->next_header = NULL;

    if (!msg->headers)
    {
        msg->headers = new_hdr;
    }
    else
    {
        protocol_header_t *last = msg->headers;
        while (last->next_header)
            last = last->next_header;
        last->next_header = new_hdr;
    }
}

bool protocols_stomp_process(protocol_buffer_t *buffer)
{
    LOG_DEBUG("Processing STOMP buffer...");

    // Ensure there is at least one message object
    if (!buffer->messages)
    {
        buffer->messages = protocols_stomp_message_initialize();
    }

    protocol_message_t *current = buffer->messages;
    while (current->next_message != NULL)
    {
        current = current->next_message;
    }

    // Loop while there is unprocessed data
    while (buffer->start < buffer->end)
    {
        char *reading = buffer->buffer + buffer->start;
        size_t remaining = buffer->end - buffer->start;

        switch (buffer->processing_stage)
        {
        case STOMP_PROCESSING_STATE_COMMAND:
        {
            LOG_DEBUG("Processing STOMP command...");
            char *command_end_ptr = strchr(reading, '\n');
            bool command_done = (command_end_ptr != NULL);

            if (!command_done)
            {
                // No newline yet – wait for more data
                LOG_DEBUG("Command incomplete, waiting for more data");
                return true; // need more data
            }

            size_t cmd_length = command_end_ptr - reading;
            size_t current_command_size = strlen(current->command);

            // Check overflow
            if (cmd_length + current_command_size >= sizeof(current->command))
            {
                LOG_ERROR("Command too long!");
                return false;
            }

            // Append the new chunk (including the newline? We'll copy without the newline)
            memcpy(current->command + current_command_size, reading, cmd_length);
            current->command[current_command_size + cmd_length] = '\0';

            // Strip trailing \r if present
            strip_cr(current->command);

            // Advance start past the newline
            buffer->start = (command_end_ptr - buffer->buffer) + 1;

            // Move to headers stage
            buffer->processing_stage = STOMP_PROCESSING_STATE_HEADERS;
            LOG_DEBUG("Command done: \"%s\"", current->command);
            break;
        }

        case STOMP_PROCESSING_STATE_HEADERS:
        {
            LOG_DEBUG("Processing STOMP headers...");
            char *line_start = buffer->buffer + buffer->start;
            char *newline = strchr(line_start, '\n');

            if (!newline)
            {
                // Incomplete header line – keep data and wait
                LOG_DEBUG("Header line incomplete, waiting for more data");
                return true;
            }

            // Compute line length (excluding newline)
            size_t line_len = newline - line_start;
            bool empty_line = (line_len == 0) || (line_len == 1 && line_start[0] == '\r');

            if (empty_line)
            {
                // End of headers – move to body
                buffer->start = (newline - buffer->buffer) + 1;
                buffer->processing_stage = STOMP_PROCESSING_STATE_BODY;
                LOG_DEBUG("Headers done, moving to body");
                break;
            }

            // We have a header line: key:value (with possible spaces)
            char *colon = strchr(line_start, ':');
            if (!colon)
            {
                LOG_ERROR("Invalid header line (missing colon): %.*s", (int)line_len, line_start);
                return false;
            }

            // Extract key (before colon)
            size_t key_len = colon - line_start;
            char key[256];
            if (key_len >= sizeof(key))
            {
                LOG_ERROR("Header key too long");
                return false;
            }
            memcpy(key, line_start, key_len);
            key[key_len] = '\0';
            trim_whitespace(key);

            // Extract value (after colon, until newline)
            size_t value_len = newline - (colon + 1);
            char value[1024]; // STOMP values can be long; adjust as needed
            if (value_len >= sizeof(value))
            {
                LOG_ERROR("Header value too long");
                return false;
            }
            memcpy(value, colon + 1, value_len);
            value[value_len] = '\0';
            trim_whitespace(value);

            // Add header to the current message
            add_header(current, key, value);
            LOG_DEBUG("Header: \"%s\"=\"%s\"", key, value);

            // Advance start past the newline
            buffer->start = (newline - buffer->buffer) + 1;
            break;
        }

        case STOMP_PROCESSING_STATE_BODY:
        {
            // Find Content-Length header
            size_t expected_len = 0;
            bool has_content_length = false;
            protocol_header_t *hdr = current->headers;
            while (hdr)
            {
                if (strcmp(hdr->key, "content-length") == 0)
                {
                    expected_len = atoi(hdr->value);
                    has_content_length = true;
                    break;
                }
                hdr = hdr->next_header;
            }

            if (!has_content_length)
            {
                // Null-terminated body
                char *null_pos = memchr(buffer->buffer + buffer->start, '\0',
                                        buffer->end - buffer->start);
                if (!null_pos)
                {
                    // No null yet – copy all remaining data and wait for more
                    size_t chunk = buffer->end - buffer->start;
                    if (chunk > 0)
                    {
                        current->body = realloc(current->body, current->body_received + chunk + 1);
                        if (!current->body)
                        { /* handle error */
                        }
                        memcpy(current->body + current->body_received,
                               buffer->buffer + buffer->start, chunk);
                        current->body_received += chunk;
                        buffer->start = buffer->end; // all consumed
                    }
                    LOG_DEBUG("Body incomplete (no null), waiting for more");
                    return true;
                }
                size_t body_part_len = null_pos - (buffer->buffer + buffer->start);
                current->body = realloc(current->body, current->body_received + body_part_len + 1);
                if (!current->body)
                { /* handle error */
                }
                memcpy(current->body + current->body_received, buffer->buffer + buffer->start, body_part_len);
                current->body_received += body_part_len;
                buffer->start = (null_pos - buffer->buffer) + 1; // skip null
                // Optionally skip trailing \r\n if any
                current->body[current->body_received] = '\0';
                current->ready = true;
                LOG_DEBUG("Body complete (null-terminated), length=%zu", current->body_received);
                // Prepare next frame
                buffer->messages->next_message = protocols_stomp_message_initialize();
                buffer->messages = buffer->messages->next_message;
                buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
                break;
            }
            else
            {
                // With Content-Length
                size_t remaining = expected_len - current->body_received;
                size_t available = buffer->end - buffer->start;
                size_t to_copy = (remaining < available) ? remaining : available;

                if (to_copy > 0)
                {
                    current->body = realloc(current->body, current->body_received + to_copy + 1);
                    if (!current->body)
                    { /* handle error */
                    }
                    memcpy(current->body + current->body_received,
                           buffer->buffer + buffer->start, to_copy);
                    current->body_received += to_copy;
                    buffer->start += to_copy;
                }

                if (current->body_received == expected_len)
                {
                    current->body[current->body_received] = '\0';
                    current->ready = true;
                    LOG_DEBUG("Body complete (Content-Length), length=%zu", current->body_received);
                    // Prepare next frame
                    buffer->messages->next_message = protocols_stomp_message_initialize();
                    buffer->messages = buffer->messages->next_message;
                    buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
                }
                else
                {
                    LOG_DEBUG("Body partial: %zu/%zu bytes", current->body_received, expected_len);
                    return true;
                }
                break;
            }
        }

        default:
            LOG_ERROR("Unknown processing stage");
            return false;
        }
    }

    // No more data to process in this buffer
    return true;
}

void protocols_stomp_initialize(protocol_buffer_t *buffer)
{
    buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
}