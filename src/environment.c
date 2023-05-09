#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "environment.h"

const char * get_user_home() {
	const char * home = getenv("HOME");
	if(!home) {
		struct passwd *pw = getpwuid(getuid());
		home = pw->pw_dir;
	}

	return home;
}

char * get_config_path() {
	static char * config_path = NULL;

	if(!config_path) {
		const char * home = get_user_home();
		
		asprintf(&config_path, "%s/.config/hif", home);
	}
	return config_path;
}

void ensure_config_path() {
	const char * home = get_user_home();
	const char * config_path = get_config_path();

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
	char * config_path = get_config_path();
	char * full_path = alloc_concat_path(config_path, context_name);
	
	struct stat st = {0};
	int exists = stat(full_path, &st) == 0; 

	free(full_path);

	return exists;
}
