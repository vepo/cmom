#include <check.h>
#include <stdio.h>

#include "core/logger.h"
#include "protocols/protocol.h"

static void assert_request_equal(protocol_request_t *expected, protocol_request_t *actual)
{
    ck_assert_int_eq(expected->command, actual->command);
    // assert headers
    if (expected->headers != NULL)
    {
        ck_assert_ptr_nonnull(actual->headers);
        protocol_header_t *current_expected_header = expected->headers;
        protocol_header_t *current_actual_header = actual->headers;
        while (current_expected_header != NULL && current_actual_header != NULL)
        {
            ck_assert_str_eq(current_expected_header->key, current_actual_header->key);
            ck_assert_str_eq(current_expected_header->value, current_actual_header->value);
            current_expected_header = current_expected_header->next_header;
            current_actual_header = current_actual_header->next_header;
            ck_assert(!(current_expected_header != NULL ^ current_actual_header != NULL));
        }
    }
    else
    {
        ck_assert_ptr_null(actual->headers);
    }
    // assert body
    if (expected->body != NULL)
    {
        ck_assert_ptr_nonnull(actual->body);
        ck_assert_str_eq(expected->body, actual->body);
    }
    else
    {
        ck_assert_ptr_null(actual->body);
    }
    ck_assert_int_eq(expected->body_len, actual->body_len);
    ck_assert_int_eq(expected->body_received, actual->body_received);
    ck_assert_int_eq(expected->ready, actual->ready);
}

static void add_message_header(protocol_request_t *req, char *key, char *value)
{
    protocol_header_t *last_header = req->headers;
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
        req->headers = current_header;
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
    memcpy(buffer->input_buffer, msg, buffer->end = track_end_of_message(msg));
}

START_TEST(parse_simple_message)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    ck_assert_ptr_nonnull(&buffer);
    ck_assert_ptr_null(buffer.requests);
    ck_assert_int_eq(0, strlen(buffer.input_buffer));
    ck_assert_int_eq(0, buffer.start);
    ck_assert_int_eq(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\ndestination:/topic/test\nmessage-id:1234\ncontent-type:text/plain\n\nHello, World!\n\0EOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    ck_assert_ptr_nonnull(buffer.requests);
    protocol_request_t expected_message = {.command = MESSAGE,
                                           .headers = NULL,
                                           .body = (char *)&"Hello, World!\n",
                                           .body_len = 14,
                                           .body_received = 14,
                                           .ready = true};
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_request_equal(&expected_message, buffer.requests);
}
END_TEST

START_TEST(parse_message_with_cr)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    ck_assert_ptr_nonnull(&buffer);
    ck_assert_ptr_null(buffer.requests);
    ck_assert_int_eq(0, strlen(buffer.input_buffer));
    ck_assert_int_eq(0, buffer.start);
    ck_assert_int_eq(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\r\ndestination:/topic/test\r\nmessage-id:1234\r\ncontent-type:text/plain\r\n\r\nHello, World!\r\n\0EOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    ck_assert_ptr_nonnull(buffer.requests);
    protocol_request_t expected_message = {.command = MESSAGE,
                                           .headers = NULL,
                                           .body = (char *)&"Hello, World!\r\n",
                                           .body_len = 15,
                                           .body_received = 15,
                                           .ready = true};
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_request_equal(&expected_message, buffer.requests);
}

START_TEST(parse_multiple_messages)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    ck_assert_ptr_nonnull(&buffer);
    ck_assert_ptr_null(buffer.requests);
    ck_assert_int_eq(0, strlen(buffer.input_buffer));
    ck_assert_int_eq(0, buffer.start);
    ck_assert_int_eq(0, buffer.end);

    reset_buffer_with_data(&buffer, "CONNECT\naccept-version:1.2\nhost:stomp.github.org\n\n\0SUBSCRIBE\nid:0\ndestination:/queue/foo\nack:client\n\n\0EOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    ck_assert_ptr_nonnull(buffer.requests);
    protocol_request_t expected_message_1 = {.command = CONNECT,
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    add_message_header(&expected_message_1, "accept-version", "1.2");
    add_message_header(&expected_message_1, "host", "stomp.github.org");
    assert_request_equal(&expected_message_1, buffer.requests);

    protocol_request_t expected_message_2 = {.command = SUBSCRIBE,
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    add_message_header(&expected_message_2, "id", "0");
    add_message_header(&expected_message_2, "destination", "/queue/foo");
    add_message_header(&expected_message_2, "ack", "client");
    assert_request_equal(&expected_message_2, buffer.requests->next_request);
}

START_TEST(parse_multiple_messages_with_heartbeat)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    ck_assert_ptr_nonnull(&buffer);
    ck_assert_ptr_null(buffer.requests);
    ck_assert_int_eq(0, strlen(buffer.input_buffer));
    ck_assert_int_eq(0, buffer.start);
    ck_assert_int_eq(0, buffer.end);

    reset_buffer_with_data(&buffer, "CONNECT\naccept-version:1.2\nhost:stomp.github.org\n\n\0\nSUBSCRIBE\nid:0\ndestination:/queue/foo\nack:client\n\n\0EOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    ck_assert_ptr_nonnull(buffer.requests);
    protocol_request_t expected_message_1 = {.command = CONNECT,
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    add_message_header(&expected_message_1, "accept-version", "1.2");
    add_message_header(&expected_message_1, "host", "stomp.github.org");
    assert_request_equal(&expected_message_1, buffer.requests);

    protocol_request_t expected_message_2 = {.command = HEARTBEAT,
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    assert_request_equal(&expected_message_2, buffer.requests->next_request);

    protocol_request_t expected_message_3 = {.command = SUBSCRIBE,
                                             .headers = NULL,
                                             .body = NULL,
                                             .body_len = 0,
                                             .body_received = 0,
                                             .ready = true};
    add_message_header(&expected_message_3, "id", "0");
    add_message_header(&expected_message_3, "destination", "/queue/foo");
    add_message_header(&expected_message_3, "ack", "client");
    assert_request_equal(&expected_message_3, buffer.requests->next_request->next_request);
}

START_TEST(content_length_chunked)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    // Send command + headers
    reset_buffer_with_data(&buffer, "MESSAGE\ncontent-length:6\n\nEOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    // Expect incomplete (body not ready)
    ck_assert(!buffer.requests->ready);

    // Send first 3 bytes of body
    reset_buffer_with_data(&buffer, "HelEOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    ck_assert_int_eq(3, buffer.requests->body_received);
    ck_assert(!buffer.requests->ready);

    // Send remaining 3 bytes
    reset_buffer_with_data(&buffer, "lo!EOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    ck_assert(buffer.requests->ready);
    ck_assert_str_eq("Hello!", buffer.requests->body);
}

START_TEST(parse_message_with_content_length_and_nulls_value)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    ck_assert_ptr_nonnull(&buffer);
    ck_assert_ptr_null(buffer.requests);
    ck_assert_int_eq(0, strlen(buffer.input_buffer));
    ck_assert_int_eq(0, buffer.start);
    ck_assert_int_eq(0, buffer.end);

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
    protocols_consume_input_buffer(STOMP, &buffer);
    ck_assert_ptr_nonnull(buffer.requests);
    protocol_request_t expected_message = {.command = MESSAGE,
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
    assert_request_equal(&expected_message, buffer.requests);
}

START_TEST(parse_simple_message_sent_in_chunks)
{
    protocol_buffer_t buffer;
    protocols_initialize(STOMP, &buffer);
    ck_assert_ptr_nonnull(&buffer);
    ck_assert_ptr_null(buffer.requests);
    ck_assert_int_eq(0, strlen(buffer.input_buffer));
    ck_assert_int_eq(0, buffer.start);
    ck_assert_int_eq(0, buffer.end);

    reset_buffer_with_data(&buffer, "MESSAGE\nEOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    ck_assert_ptr_nonnull(buffer.requests);
    protocol_request_t expected_message = {.command = MESSAGE,
                                           .headers = NULL,
                                           .body = NULL,
                                           .body_len = 0,
                                           .body_received = 0,
                                           .ready = false};
    assert_request_equal(&expected_message, buffer.requests);

    reset_buffer_with_data(&buffer, "destination:/topic/test\nmessage-id:1234\nEOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    add_message_header(&expected_message, "destination", "/topic/test");
    add_message_header(&expected_message, "message-id", "1234");
    assert_request_equal(&expected_message, buffer.requests);

    reset_buffer_with_data(&buffer, "content-type:text/plain\n\nEOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    add_message_header(&expected_message, "content-type", "text/plain");
    assert_request_equal(&expected_message, buffer.requests);

    reset_buffer_with_data(&buffer, "Hello, World!\nEOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    expected_message.body = "Hello, World!\n";
    expected_message.body_len = 14;
    expected_message.body_received = 14;
    assert_request_equal(&expected_message, buffer.requests);

    reset_buffer_with_data(&buffer, "\0EOM");
    protocols_consume_input_buffer(STOMP, &buffer);
    expected_message.body = "Hello, World!\n";
    expected_message.body_len = 14;
    expected_message.body_received = 14;
    expected_message.ready = true;
    assert_request_equal(&expected_message, buffer.requests);
}

Suite *protocols_stomp_test_suite(void)
{
    Suite *s = suite_create("Protocols :: Stomp");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, parse_simple_message);
    tcase_add_test(tc_core, parse_message_with_cr);
    tcase_add_test(tc_core, parse_multiple_messages);
    tcase_add_test(tc_core, parse_multiple_messages_with_heartbeat);
    tcase_add_test(tc_core, content_length_chunked);
    tcase_add_test(tc_core, parse_message_with_content_length_and_nulls_value);
    tcase_add_test(tc_core, parse_simple_message_sent_in_chunks);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = protocols_stomp_test_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_log(sr, "broker-tests.log");
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}