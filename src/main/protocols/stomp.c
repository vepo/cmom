#include "protocols/stomp.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

protocol_request_t *_protocols_stomp_message_initialize()
{
    protocol_request_t *req = malloc(sizeof(protocol_request_t));
    req->command = UNKNOWN;
    req->body = NULL;
    req->body_len = 0;
    req->body_received = 0;
    req->next_request = NULL;
    req->headers = NULL;
    req->ready = false;
    return req;
}

command_t _protocols_stomp_identify_command(unsigned char *command, size_t length)
{
    if (strncmp(command, "CONNECT", 7) == 0)
    {
        return CONNECT;
    }
    else if (strncmp(command, "SUBSCRIBE", 9) == 0)
    {
        return SUBSCRIBE;
    }
    else if (strncmp(command, "MESSAGE", 7) == 0)
    {
        return MESSAGE;
    }
    else
    {
        return UNKNOWN;
    }
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
static void add_header(protocol_request_t *req, const char *key, const char *value)
{
    protocol_header_t *header = malloc(sizeof(protocol_header_t));
    header->key = strdup(key);
    header->value = strdup(value);
    header->next_header = NULL;

    if (!req->headers)
    {
        req->headers = header;
    }
    else
    {
        protocol_header_t *last_header = req->headers;
        while (last_header->next_header != NULL)
        {
            last_header = last_header->next_header;
        }
        last_header->next_header = header;
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

void _protocols_stomp_process_command(protocol_buffer_t *buffer, protocol_request_t **curr_req_ptr)
{
    LOG_DEBUG("Processing STOMP command...");
    protocol_request_t *curr_req = *curr_req_ptr;

    // No data available yet
    if (buffer->start >= buffer->end)
    {
        LOG_DEBUG("Buffer empty, waiting for more data");
        return;
    }

    unsigned char *data_start = buffer->input_buffer + buffer->start;
    size_t available = buffer->end - buffer->start;

    // Search for newline within available data (safe even without null terminator)
    unsigned char *command_end_ptr = memchr(data_start, '\n', available);
    bool command_done = (command_end_ptr != NULL);

    // Check the previous data stored in working buffer.
    size_t current_len = buffer->working_buffer != NULL ? strlen(buffer->working_buffer) : 0;

    if (!command_done)
    {
        // No newline yet – copy all available data as partial command
        // Ensure we don't overflow the command buffer
        if (buffer->working_buffer != NULL)
        {
            buffer->working_buffer = realloc(buffer->working_buffer, current_len + available + 1);
        }
        else
        {
            buffer->working_buffer = malloc(sizeof(unsigned char *) * (current_len + available + 1));
        }
        memcpy(buffer->working_buffer + current_len, data_start, available);
        buffer->working_buffer[current_len + available] = '\0';
        // Consume all data; we'll wait for more
        buffer->start = buffer->end;
        LOG_DEBUG("Command partial, appended %zu bytes, waiting for more", available);
        return; // need more data
    }

    // Found a newline – complete the command line
    size_t cmd_length = command_end_ptr - data_start; // length excluding newline

    if (current_len == 0 && (cmd_length == 0 || (cmd_length == 1 && data_start[0] == '\n')))
    {
        LOG_DEBUG("Heartbeat identified!");
        curr_req->command = HEARTBEAT;
        curr_req->ready = true;
        buffer->start += cmd_length + 1;
        // Prepare next frame
        curr_req->next_request = _protocols_stomp_message_initialize();
        *curr_req_ptr = curr_req->next_request;
        buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
        return;
    }

    if (buffer->working_buffer != NULL)
    {
        buffer->working_buffer = realloc(buffer->working_buffer, current_len + cmd_length + 1);
    }
    else
    {
        buffer->working_buffer = malloc(sizeof(unsigned char *) * (current_len + cmd_length + 1));
    }

    // Append the command part (excluding newline)
    memcpy(buffer->working_buffer + current_len, data_start, cmd_length);
    buffer->working_buffer[current_len + cmd_length] = '\0';

    // Strip trailing carriage return if present
    strip_cr(buffer->working_buffer);
    curr_req->command = _protocols_stomp_identify_command(buffer->working_buffer, strlen(buffer->working_buffer));
    free(buffer->working_buffer);
    buffer->working_buffer = NULL;

    // Advance buffer start past the newline
    buffer->start = (command_end_ptr - buffer->input_buffer) + 1;

    // Move to headers processing stage
    buffer->processing_stage = STOMP_PROCESSING_STATE_HEADERS;

    LOG_DEBUG("Command complete: \"%d\"", curr_req->command);
    return; // success (command parsed)
}

void _protocols_stomp_process_headers(protocol_buffer_t *buffer, protocol_request_t **curr_req_ptr)
{
    LOG_DEBUG("Processing STOMP headers...");
    protocol_request_t *curr_req = *curr_req_ptr;

    // No data? Wait.
    if (buffer->start >= buffer->end)
    {
        LOG_DEBUG("Buffer empty, waiting for more data");
        return;
    }

    unsigned char *line_start = buffer->input_buffer + buffer->start;
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
        buffer->start = (newline - buffer->input_buffer) + 1;
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
    add_header(curr_req, key, value);
    LOG_DEBUG("Header: \"%s\"=\"%s\"", key, value);

    // Advance start past the newline
    buffer->start = (newline - buffer->input_buffer) + 1;

    return; // <-- FIX: explicit return
}

void _protocols_stomp_process_body(protocol_buffer_t *buffer, protocol_request_t **curr_req_ptr)
{
    LOG_DEBUG("Processing STOMP body...");
    protocol_request_t *curr_req = *curr_req_ptr;

    // Find Content-Length header
    size_t expected_len = 0;
    bool has_content_length = false;
    protocol_header_t *hdr = curr_req->headers;
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
        size_t remaining = expected_len - curr_req->body_received;
        size_t available = buffer->end - buffer->start;
        size_t to_copy = (remaining < available) ? remaining : available;

        if (to_copy > 0)
        {
            // Reallocate body
            char *new_body = realloc(curr_req->body, curr_req->body_received + to_copy + 1);
            if (!new_body)
            {
                LOG_ERROR("Body realloc failed");
                buffer->corrupted = true;
                return;
            }
            curr_req->body = new_body;
            memcpy(curr_req->body + curr_req->body_received, buffer->input_buffer + buffer->start, to_copy);
            curr_req->body_received += to_copy;
            buffer->start += to_copy;
        }

        if (curr_req->body_received == expected_len)
        {
            // Body complete
            curr_req->body[curr_req->body_received] = '\0';
            curr_req->body_len = curr_req->body_received;
            curr_req->ready = true;
            LOG_DEBUG("Body complete (Content-Length), length=%zu", curr_req->body_len);

            // Prepare next frame
            buffer->requests->next_request = _protocols_stomp_message_initialize();
            *curr_req_ptr = buffer->requests->next_request;
            buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
            return;
        }
        else
        {
            // Need more data
            LOG_DEBUG("Body partial: %zu/%zu bytes", curr_req->body_received, expected_len);
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

        char *data = buffer->input_buffer + buffer->start;
        int null_offset = protocols_stomp_find_body_end(data, avail); // returns offset or -1
        if (null_offset == -1)
        {
            // No null found – copy everything and wait for more
            if (avail > 0)
            {
                char *new_body = realloc(curr_req->body, curr_req->body_received + avail + 1);
                if (!new_body)
                {
                    LOG_ERROR("Body realloc failed");
                    buffer->corrupted = true;
                    return;
                }
                curr_req->body = new_body;
                memcpy(curr_req->body + curr_req->body_received, data, avail);
                curr_req->body_received += avail;
                curr_req->body_len = curr_req->body_received; // not final yet
                buffer->start = buffer->end;                  // consume all
            }
            LOG_DEBUG("Body partial (no null), waiting for more");
            return;
        }

        // Null found at offset null_offset
        size_t body_part_len = null_offset; // bytes before the null
        if (body_part_len > 0)
        {
            char *new_body;
            if (curr_req->body == NULL)
            {
                new_body = malloc(sizeof(unsigned char *) * body_part_len + 1);
            }
            else
            {
                new_body = realloc(curr_req->body, curr_req->body_received + body_part_len + 1);
            }
            if (!new_body)
            {
                buffer->corrupted = true;
                return;
            }
            curr_req->body = new_body;
            memcpy(curr_req->body + curr_req->body_received, data, body_part_len + 1);
            curr_req->body_received += body_part_len;
        }
        // Now we have the complete body (null terminator not part of body)
        if (curr_req->body != NULL)
        {
            curr_req->body[curr_req->body_received] = '\0';
        }
        curr_req->body_len = curr_req->body_received;
        curr_req->ready = true;
        LOG_DEBUG("Body complete (null-terminated), length=%zu", curr_req->body_len);

        // Advance buffer start past the null byte (and any trailing \r or \n)
        size_t consume = null_offset + 1; // skip the null
        buffer->start += consume;
        // Prepare next frame
        curr_req->next_request = _protocols_stomp_message_initialize();
        *curr_req_ptr = curr_req->next_request;
        buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
        return;
    }
}

void protocols_stomp_process(protocol_buffer_t *buffer)
{
    LOG_DEBUG("Processing STOMP buffer...");

    // Ensure there is at least one message object
    if (buffer->requests == NULL)
    {
        buffer->requests = _protocols_stomp_message_initialize();
    }

    protocol_request_t *cur_req = buffer->requests;
    while (cur_req != NULL && cur_req->next_request != NULL)
    {
        cur_req = cur_req->next_request;
    }

    // Loop while there is unprocessed data
    while (buffer->start < buffer->end && !buffer->corrupted)
    {
        char *reading = buffer->input_buffer + buffer->start;
        size_t remaining = buffer->end - buffer->start;

        switch (buffer->processing_stage)
        {
        case STOMP_PROCESSING_STATE_COMMAND:
            _protocols_stomp_process_command(buffer, &cur_req);
            break;
        case STOMP_PROCESSING_STATE_HEADERS:
            _protocols_stomp_process_headers(buffer, &cur_req);
            break;
        case STOMP_PROCESSING_STATE_BODY:
            _protocols_stomp_process_body(buffer, &cur_req);
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
        buffer->input_buffer[0] = '\0';
    }
}

void protocols_stomp_initialize(protocol_buffer_t *buffer)
{
    buffer->processing_stage = STOMP_PROCESSING_STATE_COMMAND;
}