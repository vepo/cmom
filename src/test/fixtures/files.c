#include "fixtures/files.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

char *fixtures_files_create_temp_folder()
{
    char template[] = "/tmp/tmpdir.XXXXXX";
    char *folder_name = mkdtemp(template);
    if (folder_name == NULL)
    {
        return NULL;
    }
    return strdup(folder_name); // copy to heap
}

char *fixtures_files_path(char *paths, ...)
{
    if (paths == NULL)
    {
        return strdup("");
    }

    va_list ap;
    va_start(ap, paths);

    // 1. Collect all components
    const char *components[128]; // reasonable upper bound
    int count = 0;
    components[count++] = paths;

    char *arg;
    while ((arg = va_arg(ap, char *)) != NULL)
    {
        if (count >= 128)
        {
            break; // avoid overflow
        }
        components[count++] = arg;
    }
    va_end(ap);

    // 2. Compute total length: sum of lengths + (count - 1) separators + 1 null
    size_t total_len = 0;
    for (int i = 0; i < count; i++)
    {
        total_len += strlen(components[i]);
    }
    total_len += (count - 1); // separators (slash)
    total_len += 1;           // null terminator

    // 3. Allocate and build the string
    char *result = malloc(total_len);
    if (!result)
    {
        return NULL;
    }

    char *ptr = result;
    for (int i = 0; i < count; i++)
    {
        size_t len = strlen(components[i]);
        memcpy(ptr, components[i], len);
        ptr += len;
        if (i < count - 1)
        {
            *ptr++ = '/';
        }
    }
    *ptr = '\0';

    return result;
}