#ifndef CORE_PROPERTIES_H
#define CORE_PROPERTIES_H

typedef struct properties properties_t;

properties_t *core_properties_load(char *file);
char *core_properties_value(char *key);

#endif /* CORE_PROPERTIES_H */