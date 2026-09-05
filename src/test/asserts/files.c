#include "asserts/files.h"

#include <check.h>
#include <sys/stat.h>
#include <unistd.h>

void asserts_files_exists(char *file_path)
{
    ck_assert_ptr_nonnull(file_path);
    struct stat st;
    int ret = stat(file_path, &st);
    ck_assert_int_eq(ret, 0);   // stat returns 0 if the path exists
    ck_assert_msg(S_ISREG(st.st_mode), "file is not a regular file");
}