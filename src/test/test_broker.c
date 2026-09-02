#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>

/* Example test function */
static void test_example(void)
{
    CU_ASSERT(1 == 1);
}

int main(void)
{
    CU_ErrorCode err;

    /* Initialize CUnit test registry */
    err = CU_initialize_registry();
    if (err != CUE_SUCCESS) {
        fprintf(stderr, "CUnit initialization failed\n");
        return 1;
    }

    /* Add a suite and tests */
    CU_pSuite suite = CU_add_suite("Example Suite", NULL, NULL);
    if (!suite) {
        CU_cleanup_registry();
        return 1;
    }

    if (!CU_add_test(suite, "test_example", test_example)) {
        CU_cleanup_registry();
        return 1;
    }

    /* Run tests using the basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    /* Clean up and return number of failures */
    int failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return failures;
}