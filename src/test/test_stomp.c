#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>

#include "logger.h"
#include "protocols/protocol.h"

static void assert_message_equal(protocol_message_t *expected, protocol_message_t *actual)
{
    // assert command
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

static void reset_buffer_with_data(protocol_buffer_t *buffer, char *msg, bool eom)
{
    size_t len = strlen(msg);
    buffer->start = 0;
    if (eom)
    {
        memcpy(buffer->buffer, msg, len + 1);
        buffer->end = len + 1;
    }
    else
    {
        memcpy(buffer->buffer, msg, len);
        buffer->end = len;
    }
}

static void test_parse_simple_message(void)
{
    CU_ASSERT(1 == 1);
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(&buffer);
    CU_ASSERT_PTR_NULL_FATAL(buffer.messages);
    CU_ASSERT_EQUAL_FATAL(0, strlen(buffer.buffer));
    CU_ASSERT_EQUAL_FATAL(0, buffer.start);
    CU_ASSERT_EQUAL_FATAL(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\ndestination:/topic/test\nmessage-id:1234\ncontent-type:text/plain\n\nHello, World!\n", true);
    protocols_process(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(buffer.messages);
    protocol_message_t expected_message = {.command = "MESSAGE",
                                           .headers = NULL,
                                           .body = &"Hello, World!\n",
                                           .body_len = 14,
                                           .body_received = 15, // include \0
                                           .ready = true};
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_message_equal(&expected_message, buffer.messages);
}

static void test_parse_simple_message_sent_in_chunks(void)
{
    CU_ASSERT(1 == 1);
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(&buffer);
    CU_ASSERT_PTR_NULL_FATAL(buffer.messages);
    CU_ASSERT_EQUAL_FATAL(0, strlen(buffer.buffer));
    CU_ASSERT_EQUAL_FATAL(0, buffer.start);
    CU_ASSERT_EQUAL_FATAL(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\n", false);
    protocols_process(STOMP, &buffer);
    CU_ASSERT_PTR_NOT_NULL_FATAL(buffer.messages);
    protocol_message_t expected_message = {.command = "MESSAGE",
                                           .headers = NULL,
                                           .body = NULL,
                                           .body_len = 0,
                                           .body_received = 0,
                                           .ready = false};
    assert_message_equal(&expected_message, buffer.messages);

    reset_buffer_with_data(&buffer, "destination:/topic/test\nmessage-id:1234\n", false);
    protocols_process(STOMP, &buffer);
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    assert_message_equal(&expected_message, buffer.messages);

    reset_buffer_with_data(&buffer, "content-type:text/plain\n\n", false);
    protocols_process(STOMP, &buffer);
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_message_equal(&expected_message, buffer.messages);

    reset_buffer_with_data(&buffer, "Hello, World!\n", false);
    protocols_process(STOMP, &buffer);
    expected_message.body = "Hello, World!\n";
    expected_message.body_len = 14;
    expected_message.body_received = 14;
    assert_message_equal(&expected_message, buffer.messages);

    reset_buffer_with_data(&buffer, "", true);
    protocols_process(STOMP, &buffer);
    expected_message.body = "Hello, World!\n";
    expected_message.body_len = 14;
    expected_message.body_received = 15;
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
        !CU_add_test(suite, "test_parse_simple_message_sent_in_chunks", test_parse_simple_message_sent_in_chunks))
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