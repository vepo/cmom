/* src/logger_global.c */
#include "core/logger.h"

#ifdef HAVE_LOG4C
log4c_category_t* g_log_category = NULL;
#endif