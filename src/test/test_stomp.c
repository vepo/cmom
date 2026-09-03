#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>

#include "core/logger.h"
#include "protocols/protocol.h"

static void assert_message_equal(protocol_message_t *expected, protocol_message_t *actual)
{
    CU_ASSERT_EQUAL_FATAL(0, strcmp(expected->command, actual->command));
    // assert headers
    if (expected->headers != NULL)
    {
        CU_ASSERT_PTR_NOT_NULL_FATAL(actual->headers);
        protocol_header_t *current_expected_header = expected->headers;
        protocol_header_t *current_actual_header = actual->headers;
        while (current_expected_header != NULL && current_actual_header != NULL)
        {
            CU_ASSERT_EQUAL_FATAL(0, strcmp(current_expected_header->key, current_actual_header->key));
            CU_ASSERT_EQUAL_FATAL(0, strcmp(current_expected_header->value, current_actual_header->value));
            current_expected_header = current_expected_header->next_header;
            current_actual_header = current_actual_header->next_header;
            CU_ASSERT_FALSE_FATAL(current_expected_header != NULL ^ current_actual_header != NULL);
        }
    }
    else
    {
        CU_ASSERT_PTR_NULL_FATAL(actual->headers);
    }
    // assert body
    if (expected->body != NULL)
    {
        CU_ASSERT_PTR_NOT_NULL_FATAL(actual->body);
        CU_ASSERT_EQUAL_FATAL(0, strcmp(expected->body, actual->body));
    }
    else
    {
        CU_ASSERT_PTR_NULL_FATAL(actual->body);
    }
    CU_ASSERT_EQUAL_FATAL(expected->body_len, actual->body_len);
    CU_ASSERT_EQUAL_FATAL(expected->body_received, actual->body_received);
    CU_ASSERT_EQUAL_FATAL(expected->ready, actual->ready);
}

static void add_message_header(protocol_message_t *msg, char *key, char *value)
{
    protocol_header_t *last_header = msg->headers;
    while (last_header != NULL && last_header->next_header != NULL)
    {
        last_header = last_header->next_header;
    }
    protocol_header_t *current_header = malloc(sizeof(protocol_header_t));
    current_header->key = key;
    current_header->value = value;
    current_header->next_header = NULL;
    if (last_header == NULL)
    {
        msg->headers = current_header;
    }
    else
    {
        last_header->next_header = current_header;
    }
}

size_t track_end_of_message(unsigned char *msg)
{
    size_t length = 0;
    while (!(msg[length] == 'E' && msg[length + 1] == 'O' && msg[length + 2] == 'M'))
    {
        length++;
    }
    return length;
}

static void reset_buffer_with_data(protocol_buffer_t *buffer, unsigned char *msg)
{
    buffer->start = 0;
    memcpy(buffer->buffer, msg, buffer->end = track_end_of_message(msg));
}

static void test_parse_simple_message(void)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(&buffer);
    CU_ASSERT_PTR_NULL_FATAL(buffer.messages);
    CU_ASSERT_EQUAL_FATAL(0, strlen(buffer.buffer));
    CU_ASSERT_EQUAL_FATAL(0, buffer.start);
    CU_ASSERT_EQUAL_FATAL(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\ndestination:/topic/test\nmessage-id:1234\ncontent-type:text/plain\n\nHello, World!\n\0EOM");
    protocols_process(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(buffer.messages);
    protocol_message_t expected_message = {.command = "MESSAGE",
                                           .headers = NULL,
                                           .body = (char *)&"Hello, World!\n",
                                           .body_len = 14,
                                           .body_received = 14,
                                           .ready = true};
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_message_equal(&expected_message, buffer.messages);
}

static void test_parse_message_with_cr(void)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(&buffer);
    CU_ASSERT_PTR_NULL_FATAL(buffer.messages);
    CU_ASSERT_EQUAL_FATAL(0, strlen(buffer.buffer));
    CU_ASSERT_EQUAL_FATAL(0, buffer.start);
    CU_ASSERT_EQUAL_FATAL(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\r\ndestination:/topic/test\r\nmessage-id:1234\r\ncontent-type:text/plain\r\n\r\nHello, World!\r\n\0EOM");
    protocols_process(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(buffer.messages);
    protocol_message_t expected_message = {.command = "MESSAGE",
                                           .headers = NULL,
                                           .body = (char *)&"Hello, World!\r\n",
                                           .body_len = 15,
                                           .body_received = 15,
                                           .ready = true};
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_message_equal(&expected_message, buffer.messages);
}

static void test_parse_multiple_messages(void)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(&buffer);
    CU_ASSERT_PTR_NULL_FATAL(buffer.messages);
    CU_ASSERT_EQUAL_FATAL(0, strlen(buffer.buffer));
    CU_ASSERT_EQUAL_FATAL(0, buffer.start);
    CU_ASSERT_EQUAL_FATAL(0, buffer.end);

    reset_buffer_with_data(&buffer, "CONNECT\naccept-version:1.2\nhost:stomp.github.org\n\n\0SUBSCRIBE\nid:0\ndestination:/queue/foo\nack:client\n\n\0EOM");
    protocols_process(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(buffer.messages);
    protocol_message_t expected_message_1 = {.command = "CONNECT",
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    add_message_header(&expected_message_1, "accept-version", "1.2");
    add_message_header(&expected_message_1, "host", "stomp.github.org");
    assert_message_equal(&expected_message_1, buffer.messages);

    protocol_message_t expected_message_2 = {.command = "SUBSCRIBE",
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    add_message_header(&expected_message_2, "id", "0");
    add_message_header(&expected_message_2, "destination", "/queue/foo");
    add_message_header(&expected_message_2, "ack", "client");
    assert_message_equal(&expected_message_2, buffer.messages->next_message);
}

static void test_parse_multiple_messages_with_heartbeat(void)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(&buffer);
    CU_ASSERT_PTR_NULL_FATAL(buffer.messages);
    CU_ASSERT_EQUAL_FATAL(0, strlen(buffer.buffer));
    CU_ASSERT_EQUAL_FATAL(0, buffer.start);
    CU_ASSERT_EQUAL_FATAL(0, buffer.end);

    reset_buffer_with_data(&buffer, "CONNECT\naccept-version:1.2\nhost:stomp.github.org\n\n\0\nSUBSCRIBE\nid:0\ndestination:/queue/foo\nack:client\n\n\0EOM");
    protocols_process(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(buffer.messages);
    protocol_message_t expected_message_1 = {.command = "CONNECT",
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    add_message_header(&expected_message_1, "accept-version", "1.2");
    add_message_header(&expected_message_1, "host", "stomp.github.org");
    assert_message_equal(&expected_message_1, buffer.messages);


    protocol_message_t expected_message_2 = {.command = "HEARTBEAT",
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    assert_message_equal(&expected_message_2, buffer.messages->next_message);

    protocol_message_t expected_message_3 = {.command = "SUBSCRIBE",
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    add_message_header(&expected_message_3, "id", "0");
    add_message_header(&expected_message_3, "destination", "/queue/foo");
    add_message_header(&expected_message_3, "ack", "client");
    assert_message_equal(&expected_message_3, buffer.messages->next_message->next_message);
}

static void test_content_length_chunked(void)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    // Send command + headers
    reset_buffer_with_data(&buffer, "MESSAGE\ncontent-length:6\n\nEOM");
    protocols_process(STOMP, &buffer);
    // Expect incomplete (body not ready)
    CU_ASSERT_FALSE(buffer.messages->ready);

    // Send first 3 bytes of body
    reset_buffer_with_data(&buffer, "HelEOM");
    protocols_process(STOMP, &buffer);
    CU_ASSERT_EQUAL(buffer.messages->body_received, 3);
    CU_ASSERT_FALSE(buffer.messages->ready);

    // Send remaining 3 bytes
    reset_buffer_with_data(&buffer, "lo!EOM");
    protocols_process(STOMP, &buffer);
    CU_ASSERT_TRUE(buffer.messages->ready);
    CU_ASSERT_STRING_EQUAL(buffer.messages->body, "Hello!");
}

static void test_parse_message_with_content_length_and_nulls_value(void)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(&buffer);
    CU_ASSERT_PTR_NULL_FATAL(buffer.messages);
    CU_ASSERT_EQUAL_FATAL(0, strlen(buffer.buffer));
    CU_ASSERT_EQUAL_FATAL(0, buffer.start);
    CU_ASSERT_EQUAL_FATAL(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\ndestination:/topic/test\nmessage-id:1234\ncontent-length:10\ncontent-type:text/plain\n\n"
                                    "\0"
                                    "1"
                                    "\0"
                                    "2"
                                    "\0"
                                    "3"
                                    "\0"
                                    "4"
                                    "\0"
                                    "5EOM");
    protocols_process(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(buffer.messages);
    protocol_message_t expected_message = {.command = "MESSAGE",
                                           .headers = NULL,
                                           .body = (unsigned char *)&"\0"
                                                                     "1"
                                                                     "\0"
                                                                     "2"
                                                                     "\0"
                                                                     "3"
                                                                     "\0"
                                                                     "4"
                                                                     "\0"
                                                                     "5",
                                           .body_len = 10,
                                           .body_received = 10,
                                           .ready = true};
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    add_message_header(&expected_message, "content-length", "10");
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_message_equal(&expected_message, buffer.messages);
}

static void test_parse_simple_message_sent_in_chunks(void)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(&buffer);
    CU_ASSERT_PTR_NULL_FATAL(buffer.messages);
    CU_ASSERT_EQUAL_FATAL(0, strlen(buffer.buffer));
    CU_ASSERT_EQUAL_FATAL(0, buffer.start);
    CU_ASSERT_EQUAL_FATAL(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\nEOM");
    protocols_process(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(buffer.messages);
    protocol_message_t expected_message = {.command = "MESSAGE",
                                           .headers = NULL,
                                           .body = NULL,
                                           .body_len = 0,
                                           .body_received = 0,
                                           .ready = false};
    assert_message_equal(&expected_message, buffer.messages);

    reset_buffer_with_data(&buffer, "destination:/topic/test\nmessage-id:1234\nEOM");
    protocols_process(STOMP, &buffer);
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    assert_message_equal(&expected_message, buffer.messages);

    reset_buffer_with_data(&buffer, "content-type:text/plain\n\nEOM");
    protocols_process(STOMP, &buffer);
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_message_equal(&expected_message, buffer.messages);

    reset_buffer_with_data(&buffer, "Hello, World!\nEOM");
    protocols_process(STOMP, &buffer);
    expected_message.body = "Hello, World!\n";
    expected_message.body_len = 14;
    expected_message.body_received = 14;
    assert_message_equal(&expected_message, buffer.messages);

    reset_buffer_with_data(&buffer, "\0EOM");
    protocols_process(STOMP, &buffer);
    expected_message.body = "Hello, World!\n";
    expected_message.body_len = 14;
    expected_message.body_received = 14;
    expected_message.ready = true;
    assert_message_equal(&expected_message, buffer.messages);
}

int main(void)
{
    LOGGER_INIT();
    CU_ErrorCode err;

    /* Initialize CUnit test registry */
    err = CU_initialize_registry();
    if (err != CUE_SUCCESS)
    {
        fprintf(stderr, "CUnit initialization failed\n");
        return 1;
    }

    /* Add a suite and tests */
    CU_pSuite suite = CU_add_suite("Example Suite", NULL, NULL);
    if (!suite)
    {
        CU_cleanup_registry();
        return 1;
    }

    if (!CU_add_test(suite, "test_parse_simple_message", test_parse_simple_message) ||
        !CU_add_test(suite, "test_parse_message_with_cr", test_parse_message_with_cr) ||
        !CU_add_test(suite, "test_parse_simple_message_sent_in_chunks", test_parse_simple_message_sent_in_chunks) ||
        !CU_add_test(suite, "test_parse_message_with_content_length_and_nulls_value", test_parse_message_with_content_length_and_nulls_value) ||
        !CU_add_test(suite, "test_content_length_chunked", test_content_length_chunked) ||
        !CU_add_test(suite, "test_parse_multiple_messages", test_parse_multiple_messages) ||
        !CU_add_test(suite, "test_parse_multiple_messages_with_heartbeat", test_parse_multiple_messages_with_heartbeat))
    {
        CU_cleanup_registry();
        return 1;
    }

    /* Run tests using the basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    /* Clean up and return number of failures */
    int failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    LOGGER_CLEANUP();
    return failures;
}