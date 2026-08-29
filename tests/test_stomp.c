#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "protocols/protocol.h"
#include "protocols/stomp.h"
#include <string.h>
#include <stdio.h>

static protocol_buffer_t buffer;

static void test_stomp_parse_command(void)
{
    const char *frame = "CONNECT\naccept-version:1.2\n\n\0";
    size_t len = strlen(frame) + 1; // include null terminator

    memcpy(buffer.buffer, frame, len);
    buffer.start = 0;
    buffer.end = len;
    buffer.processing_stage = STOMP_PROCESSING_STATE_COMMAND;
    buffer.messages = NULL;
    protocols_stomp_initialize(&buffer);

    bool result = protocols_stomp_process(&buffer);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_PTR_NOT_NULL(buffer.messages);
    CU_ASSERT_STRING_EQUAL(buffer.messages->command, "CONNECT");
}

int main(void)
{
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Stomp Parser", NULL, NULL);
    CU_add_test(suite, "parse CONNECT", test_stomp_parse_command);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}