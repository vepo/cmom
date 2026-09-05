#include <check.h>

#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>

#include "core/properties.h"
#include "fixtures/files.h"
#include "storage/engine.h"

START_TEST(initialize_storage_engine)
{
    ck_assert(true);
    char *temp_folder = fixtures_files_create_temp_folder();
    storage_engine_init(temp_folder);

    char * meta_path = fixtures_files_path(temp_folder, "meta.properties");    
    properties_t * meta_properties = core_properties_load(meta_path);    
    free(meta_path);

    ck_assert_ptr_nonnull(meta_properties);
    if (rmdir(temp_folder) == -1)
    {
        perror("rmdir failed");
    }
}
END_TEST

Suite *protocols_stomp_test_suite(void)
{
    Suite *s = suite_create("Storage :: Engine");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, initialize_storage_engine);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = protocols_stomp_test_suite();
    SRunner *sr = srunner_create(s);

    srunner_set_log(sr, "tests-storage.log");
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}