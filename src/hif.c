#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

#include "hif.h"
#include "environment.h"
#include "storage_adapter.h"

typedef int (*fn_command)(sqlite3 * db, void * payload);

static void print_version(FILE * out) {
	fprintf(out, HIF_EXECUTABLE " " HIF_VERSION "\n");
}

static void print_help(FILE * out) {
	print_version(out);	
	fprintf(out, "usage: hif [mood | command (args)*]\n\n");
	if(out == stderr) {
		fprintf(out, "Sorry bud, you need to tell me how you feel.\n");
		fprintf(out, "By default, I support the following moods:\n");
		fprintf(out, "\tbad, meh, woo\n\n");
		fprintf(out, "... but you can create your own. Example:\n");
		fprintf(out, "\t$ hif add woo\n");
		fprintf(out, "\nOr try a command:\n");
	}
	fprintf(out, "\tadd {feel}     - Add a new feel.\n");
	fprintf(out, "\tjson           - Dump feels in json format.\n");
	fprintf(out, "\tdelete {id}    - Delete feel by id, i.e:\n\t\t$ hif delete 1\n\n");
	fprintf(out, "\tcount          - Return a count of feels.\n");
	fprintf(out, "\tcreate-context - Create a new feels context database.\n");
	fprintf(out, "\tcreate-feel    - Create a new feel.\n");
	fprintf(out, "\thelp           - Print this message.\n");
	fprintf(out, "\tversion        - Print hif version information.\n");
	fprintf(out, "\n");
}

static int create_table_fn(void * data, int argc, char **argv, char **col) {
	return 0;
}

static int select_fn(void *data, int argc, char **argv, char **col){
	fprintf(stdout, "\t\t{");
   	for(int i = 0; i<argc; i++){
		fprintf(stdout, "'%s': '%s'", col[i], argv[i] ? argv[i] : "NULL");
		if(i < argc - 1) {
			fprintf(stdout, ", ");
		}
   	}
	fprintf(stdout, "}\n");
   	return 0;
}

static char * str_lower(char * s) {
	char * p = s;
	for (; *p; ++p) { *p = tolower(*p); }
	return s;
}

static hif_command str_to_command(const char * s) {
	hif_command feel;
	if(strncmp(s, "add", sizeof("add") - 1) == 0) {
		return HIF_COMMAND | HIF_COMMAND_ADD_FEEL;
	} else if(strncmp(s, "json", sizeof("json") - 1) == 0) {
		return HIF_COMMAND | HIF_COMMAND_JSON;
	} else if(strncmp(s, "delete", sizeof("delete") - 1) == 0) {
		return HIF_COMMAND | HIF_COMMAND_DELETE_FEEL;
	} else if(strncmp(s, "count", sizeof("count") - 1) == 0) {
		return HIF_COMMAND | HIF_COMMAND_COUNT;
	} else if(strncmp(s, "create-context", sizeof("create-context") - 1) == 0) {
		return HIF_COMMAND | HIF_COMMAND_CREATE;
	} else if(strncmp(s, "create-feel", sizeof("create-feel") - 1) == 0) {
		return HIF_COMMAND | HIF_COMMAND_CREATE_FEEL;
	} else if(strncmp(s, "help", sizeof("help") - 1) == 0) {
		print_help(stdout);
		exit(0);
	} else if(strncmp(s, "version", sizeof("version") - 1) == 0) {
		print_version(stdout);
		exit(0);
	} else {
		print_help(stderr);
		exit(-1);
	}

	exit(-1);
}

static void terminate() {
	char * config_path = get_config_path();
	free(config_path);
}

static int initialize() {
	int ret = atexit(terminate);
	ensure_config_path();
	
	return ret;
}

int main(int argc, char **argv) {
	sqlite3 * db = NULL;
	char * err_msg = NULL, * sql = NULL;
	int rc = 0, ret = -1;
	
	int call_terminate_on_exit = initialize();

	if(argc < 2) {
		print_help(stderr);
		goto err0;
	}

	char *p = str_lower(argv[1]);

	sqlite3_initialize();

	hif_command command = str_to_command(p);
	
	storage_adapter const * adapter = storage_adapter_alloc();
	if(!context_exists("hif.db")) {
		adapter->create_storage(adapter, "hif.db");
	}
	adapter->open_storage(adapter, "hif.db");

	if(command & HIF_COMMAND_CREATE) {
		const char * path = NULL;
		if(argc >= 3) {
			path = argv[2];
		}
		adapter->create_storage(adapter, path);
		fprintf(stdout, "Created context %s\n", path);
	}
	else if(command & HIF_COMMAND_ADD_FEEL) {
		char * description = NULL;
		adapter->insert_feel(adapter, argv[2], &description);
		fprintf(stdout, "Feels added %s\n", description ? description : ":)");
	}
	else if(command & HIF_COMMAND_COUNT) {
		int count = adapter->count_feels(adapter);
		fprintf(stdout, "%i\n", count);
	}
	else if(command & HIF_COMMAND_JSON) {
		adapter->export(adapter, NULL);
	}
	else if(command & HIF_COMMAND_DELETE_FEEL) {
		if(argc < 3) {
			print_help(stderr);
			goto err1;
		}
		int id = atoi(argv[2]);
		adapter->delete_feel(adapter, id);
	}
	else if(command & HIF_COMMAND_CREATE_FEEL) {
		if(argc < 3) {
			print_help(stderr);
			goto err1;
		}
		char const * name = argv[2];
		char const * description = NULL;
		if(argc > 3) {
			description = argv[3];	
		}

		adapter->create_feel(adapter, name, description);
		fprintf(stdout, "Created feel '%s': %s\n", name, description ? description : "[empty]");
	}
	else {
		fprintf(stderr, "What?\n");
		/* rc = command(db, NULL); */
	}

	adapter->close(adapter);
	adapter->free((storage_adapter *)adapter);

	goto err1;

	ret = 0;
err3:
err2:
err1:
	sqlite3_shutdown();

err0:
	if(call_terminate_on_exit) {
		terminate();
	}
	return ret;
}
