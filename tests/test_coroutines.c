#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "coroutines.h"
#include "event_watch.h"
#include <ucontext.h>
#include <string.h>
#include <stdlib.h>

/* Global counter used by the coroutine to signal it ran. */
static int coroutine_ran = 0;

/* Dummy broker context with minimal setup. */
static broker_context_t test_broker;
static ucontext_t main_ctx;

/* The coroutine function – it will increment the counter and yield back. */
static void test_coroutine_fn(connection_context_t *conn)
{
    coroutine_ran++;
    // Yield back to the broker (main context)
    coroutines_leave_connection(conn);
    // After yield, when resumed again, increment once more
    coroutine_ran++;
}

/* Test that a coroutine can be scheduled, joined, and yields. */
static void test_coroutine_schedule_and_join(void)
{
    // Save the current context (the test runner's context) to use as broker context.
    // We'll set test_broker.context to the main context of this test.
    getcontext(&test_broker.context);
    // We also need a valid event_watch (even if not used) to avoid null deref.
    test_broker.event_watch.io_poll = -1; // dummy
    // Connections array is global; we can leave it zero-initialized.
    memset(test_broker.connections, 0, sizeof(test_broker.connections));

    // Create a dummy socket fd (we don't actually use it for I/O).
    int dummy_socket = 42;

    // Schedule the coroutine.
    connection_context_t *conn = coroutines_schedule(dummy_socket, &test_broker,
                                                     (void*)test_coroutine_fn);
    CU_ASSERT_PTR_NOT_NULL(conn);

    // Initially the coroutine has not run.
    coroutine_ran = 0;

    // Join the coroutine – this will run it until it yields.
    coroutines_join_connection(conn);
    // After join, the coroutine has run and yielded once.
    CU_ASSERT_EQUAL(coroutine_ran, 1);

    // Now join again – the coroutine resumes from where it yielded and increments again.
    coroutines_join_connection(conn);
    CU_ASSERT_EQUAL(coroutine_ran, 2);

    // Clean up: free the stack and the connection.
    free(conn->context.uc_stack.ss_sp);
    free(conn);
    // Clean up the dummy socket entry.
    test_broker.connections[dummy_socket] = NULL;
}

int main(void)
{
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Coroutines", NULL, NULL);
    CU_add_test(suite, "schedule and join", test_coroutine_schedule_and_join);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}