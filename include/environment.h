#ifndef HIF_ENVIRONMENT
#define HIF_ENVIRONMENT

#include <stdio.h>

char const * get_user_home();
char const * get_config_path();
void ensure_config_path();
char * alloc_concat_path(char const * root_path, char const * path);

int context_exists(char const * context_name);

#ifndef asprintf
int asprintf(char **ret, const char *format, ...);
#endif

#endif /* HIF_ENVIRONMENT */
