#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "environment.h"

#ifndef asprintf
int asprintf(char **ret, const char *format, ...) {
  va_list ap;
  *ret = NULL;

  va_start(ap, format);
  int count = vsnprintf(NULL, 0, format, ap);
  va_end(ap);

  if(count >= 0) {
    char * buffer = malloc(count + 1);
    if(!buffer) return -1;

    va_start(ap, format);
    count = vsnprintf(buffer, count + 1, format, ap);
    va_end(ap);

    if(count < 0) {
      free(buffer), buffer = NULL;
      return count;
    }
    
    *ret = buffer;
  }
  return count;
}
#endif

char const * get_user_home() {
  char const * home = getenv("HOME");
  if(!home) {
    struct passwd *pw = getpwuid(getuid());
    home = pw->pw_dir;
  }

  return home;
}

char const * get_config_path() {
  static char * config_path = NULL;

  if(!config_path) {
    char const * home = get_user_home();
    
    asprintf(&config_path, "%s/.config/hif", home);
  }
  return config_path;
}

void ensure_config_path() {
  char const * config_path = get_config_path();

  struct stat st = {0};
  if (stat(config_path, &st) == -1) {
    mkdir(config_path, 0700);
  }
}

char * alloc_concat_path(char const * root_path, char const * path) {
  char * full_path = malloc(strlen(root_path) + strlen(path) + sizeof('/') + sizeof('\0'));
  if (!full_path) { abort(); }
  sprintf(full_path, "%s/%s", root_path, path);

  return full_path;
}

int context_exists(char const * context_name) {
  char const * config_path = get_config_path();
  char * full_path = alloc_concat_path(config_path, context_name);
  
  struct stat st = {0};
  int exists = stat(full_path, &st) == 0; 

  free(full_path), full_path = NULL;

  return exists;
}
