#include "core/properties.h"

#include <stdlib.h>

typedef struct properties_item
{
    char *key;
    char *value;
    properties_t *next;
} properties_item_t;

typedef struct properties
{
    properties_item_t *values;
} properties_t;

properties_t *core_properties_load(char *file) {
    properties_t * properties = malloc(sizeof(properties_t));
    return properties;
}
char *core_properties_value(char *key) {
    return NULL;
}
