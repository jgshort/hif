#ifndef HIF_ENVIRONMENT
#define HIF_ENVIRONMENT

const char * get_user_home();
char * get_config_path();
void ensure_config_path();
char * alloc_concat_path(char const * root_path, char const * path);

int context_exists(char const * context_name);

#endif /* HIF_ENVIRONMENT */
