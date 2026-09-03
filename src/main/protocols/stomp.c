#include "protocols/stomp.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

protocol_message_t *_protocols_stomp_message_initialize()
{
    protocol_message_t *message = malloc(sizeof(protocol_message_t));
    message->command[0] = '\0';
    message->body = NULL;
    message->body_len = 0;
    message->body_received = 0;
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
    {
        str++;
    }
    // Trim trailing
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r'))
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
        {
            last = last->next_header;
        }
        last->next_header = new_hdr;
    }
}

int protocols_stomp_find_body_end(char *buffer, size_t length)
{
    LOG_DEBUG("Trying to find buffer end!");
    for (size_t pos = 0; pos < length; ++pos)
    {
        if (buffer[pos] == '\0')
        {
            return pos;
        }
    }
    return -1;
}

void _protocols_stomp_process_command(protocol_buffer_t *buffer, protocol_message_t **current_ptr)
{
    LOG_DEBUG("Processing STOMP command...");
    protocol_message_t *current = *current_ptr;

    // No data available yet
    if (buffer->start >= buffer->end)
    {
        LOG_DEBUG("Buffer empty, waiting for more data");
        buffer->corrupted = true;
        return; // need more data
    }

    unsigned char *data_start = buffer->buffer + buffer->start;
    size_t available = buffer->end - buffer->start;

    // Search for newline within available data (safe even without null terminator)
    unsigned char *command_end_ptr = memchr(data_start, '\n', available);
    bool command_done = (command_end_ptr != NULL);

    if (!command_done)
    {
        // No newline yet – copy all available data as partial command
        size_t current_len = strlen(current->command);
        // Ensure we don't overflow the command buffer
        if (current_len + available >= sizeof(current->command))
        {
            LOG_ERROR("Command too long (partial)!");
            buffer->corrupted = true;
            return;
        }
        memcpy(current->command + current_len, data_start, available);
        current->command[current_len + available] = '\0';
        // Consume all data; we'll wait for more
        buffer->start = buffer->end;
        LOG_DEBUG("Command partial, appended %zu bytes, waiting for more", available);
        return; // need more data
    }

    // Found a newline – complete the command line
    size_t cmd_length = command_end_ptr - data_start; // length excluding newline
    size_t current_len = strlen(current->command);

    if (cmd_length == 0 || (cmd_length == 1 && data_start[0] == '\n' )) {
        LOG_DEBUG("Heartbeat identified!");
        memcpy(&current->command, "HEARTBEAT\0", sizeof(char) * 10);
        current->ready = true;
        buffer->start += cmd_length + 1;
        // Prepare next frame
        current->next_message = _protocols_stomp_message_initialize();
        *current_ptr = current->next_message;
        buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
        return;
    }

    // Check overflow (including null terminator)
    if (current_len + cmd_length >= sizeof(current->command))
    {
        LOG_ERROR("Command too long (complete)!");
        buffer->corrupted = true;
        return;
    }

    // Append the command part (excluding newline)
    memcpy(current->command + current_len, data_start, cmd_length);
    current->command[current_len + cmd_length] = '\0';

    // Strip trailing carriage return if present
    strip_cr(current->command);

    // Advance buffer start past the newline
    buffer->start = (command_end_ptr - buffer->buffer) + 1;

    // Move to headers processing stage
    buffer->processing_stage = STOMP_PROCESSING_STATE_HEADERS;

    LOG_DEBUG("Command complete: \"%s\"", current->command);
    return; // success (command parsed)
}

void _protocols_stomp_process_headers(protocol_buffer_t *buffer, protocol_message_t **current_ptr)
{
    LOG_DEBUG("Processing STOMP headers...");
    protocol_message_t *current = *current_ptr;

    // No data? Wait.
    if (buffer->start >= buffer->end)
    {
        LOG_DEBUG("Buffer empty, waiting for more data");
        return;
    }

    unsigned char *line_start = buffer->buffer + buffer->start;
    size_t avail = buffer->end - buffer->start;

    // Search for newline within available bytes
    unsigned char *newline = memchr(line_start, '\n', avail);
    if (!newline)
    {
        // Incomplete header line – keep data and wait
        LOG_DEBUG("Header line incomplete, waiting for more data");
        return;
    }

    // Line length excludes the newline
    size_t line_len = newline - line_start;
    bool empty_line = (line_len == 0) || (line_len == 1 && line_start[0] == '\r');

    if (empty_line)
    {
        // End of headers – move to body
        buffer->start = (newline - buffer->buffer) + 1;
        buffer->processing_stage = STOMP_PROCESSING_STATE_BODY;
        LOG_DEBUG("Headers done, moving to body");
        return;
    }

    // Find colon within this line only
    unsigned char *colon = memchr(line_start, ':', line_len);
    if (!colon)
    {
        LOG_ERROR("Invalid header line (missing colon): %.*s", (int)line_len, line_start);
        buffer->corrupted = true;
        return;
    }

    // Extract key (before colon)
    size_t key_len = colon - line_start;
    if (key_len >= sizeof(current->command))
    { // reuse command size or define HEADER_KEY_MAX
        LOG_ERROR("Header key too long");
        buffer->corrupted = true;
        return;
    }
    char key[256];
    memcpy(key, line_start, key_len);
    key[key_len] = '\0';
    trim_whitespace(key);

    // Extract value (after colon, until newline)
    size_t value_len = newline - (colon + 1);
    if (value_len >= 1024)
    { // or a constant
        LOG_ERROR("Header value too long");
        buffer->corrupted = true;
        return;
    }
    char value[1024];
    memcpy(value, colon + 1, value_len);
    value[value_len] = '\0';
    trim_whitespace(value);

    // Add header to message
    add_header(current, key, value);
    LOG_DEBUG("Header: \"%s\"=\"%s\"", key, value);

    // Advance start past the newline
    buffer->start = (newline - buffer->buffer) + 1;

    return; // <-- FIX: explicit return
}

void _protocols_stomp_process_body(protocol_buffer_t *buffer, protocol_message_t **current_ptr)
{
    LOG_DEBUG("Processing STOMP body...");
    protocol_message_t *current = *current_ptr;

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
            LOG_DEBUG("Message has content-length: %zu", expected_len);
            break;
        }
        hdr = hdr->next_header;
    }

    if (has_content_length)
    {
        // ---------- Content-Length mode ----------
        size_t remaining = expected_len - current->body_received;
        size_t available = buffer->end - buffer->start;
        size_t to_copy = (remaining < available) ? remaining : available;

        if (to_copy > 0)
        {
            // Reallocate body
            char *new_body = realloc(current->body, current->body_received + to_copy + 1);
            if (!new_body)
            {
                LOG_ERROR("Body realloc failed");
                buffer->corrupted = true;
                return;
            }
            current->body = new_body;
            memcpy(current->body + current->body_received, buffer->buffer + buffer->start, to_copy);
            current->body_received += to_copy;
            buffer->start += to_copy;
        }

        if (current->body_received == expected_len)
        {
            // Body complete
            current->body[current->body_received] = '\0';
            current->body_len = current->body_received;
            current->ready = true;
            LOG_DEBUG("Body complete (Content-Length), length=%zu", current->body_len);

            // Prepare next frame
            buffer->messages->next_message = _protocols_stomp_message_initialize();
            *current_ptr = buffer->messages->next_message;
            buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
            return;
        }
        else
        {
            // Need more data
            LOG_DEBUG("Body partial: %zu/%zu bytes", current->body_received, expected_len);
            return; // not an error
        }
    }
    else
    {
        // ---------- Null-terminated mode ----------
        size_t avail = buffer->end - buffer->start;
        if (avail == 0)
        {
            LOG_DEBUG("No body data available, waiting");
            return;
        }

        char *data = buffer->buffer + buffer->start;
        int null_offset = protocols_stomp_find_body_end(data, avail); // returns offset or -1
        if (null_offset == -1)
        {
            // No null found – copy everything and wait for more
            if (avail > 0)
            {
                char *new_body = realloc(current->body, current->body_received + avail + 1);
                if (!new_body)
                {
                    LOG_ERROR("Body realloc failed");
                    buffer->corrupted = true;
                    return;
                }
                current->body = new_body;
                memcpy(current->body + current->body_received, data, avail);
                current->body_received += avail;
                current->body_len = current->body_received; // not final yet
                buffer->start = buffer->end;                // consume all
            }
            LOG_DEBUG("Body partial (no null), waiting for more");
            return;
        }

        // Null found at offset null_offset
        size_t body_part_len = null_offset; // bytes before the null
        if (body_part_len > 0)
        {
            char *new_body;
            if (current->body == NULL)
            {
                new_body = malloc(sizeof(unsigned char *) * body_part_len + 1);
            }
            else
            {
                new_body = realloc(current->body, current->body_received + body_part_len + 1);
            }
            if (!new_body)
            {
                buffer->corrupted = true;
                return;
            }
            current->body = new_body;
            memcpy(current->body + current->body_received, data, body_part_len + 1);
            current->body_received += body_part_len;
        }
        // Now we have the complete body (null terminator not part of body)
        if (current->body != NULL)
        {
            current->body[current->body_received] = '\0';
        }
        current->body_len = current->body_received;
        current->ready = true;
        LOG_DEBUG("Body complete (null-terminated), length=%zu", current->body_len);

        // Advance buffer start past the null byte (and any trailing \r or \n)
        size_t consume = null_offset + 1; // skip the null
        buffer->start += consume;
        // Prepare next frame
        current->next_message = _protocols_stomp_message_initialize();
        *current_ptr = current->next_message;
        buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
        return;
    }
}

void protocols_stomp_process(protocol_buffer_t *buffer)
{
    LOG_DEBUG("Processing STOMP buffer...");

    // Ensure there is at least one message object
    if (buffer->messages == NULL)
    {
        buffer->messages = _protocols_stomp_message_initialize();
    }

    protocol_message_t *current = buffer->messages;
    while (current != NULL && current->next_message != NULL)
    {
        current = current->next_message;
    }

    // Loop while there is unprocessed data
    while (buffer->start < buffer->end && !buffer->corrupted)
    {
        char *reading = buffer->buffer + buffer->start;
        size_t remaining = buffer->end - buffer->start;

        switch (buffer->processing_stage)
        {
        case STOMP_PROCESSING_STATE_COMMAND:
            _protocols_stomp_process_command(buffer, &current);
            break;
        case STOMP_PROCESSING_STATE_HEADERS:
            _protocols_stomp_process_headers(buffer, &current);
            break;
        case STOMP_PROCESSING_STATE_BODY:
            _protocols_stomp_process_body(buffer, &current);
            break;
        default:
            LOG_ERROR("Unknown processing stage");
            buffer->corrupted = true;
            return;
        }
    }

    // No more data to process in this buffer
    if (buffer->start == buffer->end)
    {
        buffer->start = 0;
        buffer->end = 0;
        buffer->buffer[0] = '\0';
    }
}

void protocols_stomp_initialize(protocol_buffer_t *buffer)
{
    buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
}