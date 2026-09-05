#ifndef FIXTURES_FILES_H
#define FIXTURES_FILES_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Forward declaration */
char *fixtures_files_create_temp_folder(void);
char *fixtures_files_path(char *paths, ...);

#endif /* FIXTURES_FILES_H */