#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

#include "hif.h"
#include "environment.h"
#include "memo_repository.h"
#include "storage_adapter.h"

typedef int (*fn_command)(sqlite3 * db, void * payload);

static void print_version(FILE * out) {
	fprintf(out, HIF_EXECUTABLE " " HIF_VERSION "\n");
}

static void print_help(FILE * out) {
	print_version(out);	
	fprintf(out, "usage: hif [+emotion | command (args)*]\n\n");
	if(out == stderr) {
		fprintf(out, "Sorry bud, you need to tell me how you feel.\n");
		fprintf(out, "\nOr try a command:\n");
	}
	fprintf(out, "\nEmotion Commands\n");
	fprintf(out, "\tadd {emotion}        - Journal a new {emotion} feel.\n");
	fprintf(out, "\t                       alias +, i.e. $ hif +sad\n");
	fprintf(out, "\tdelete-feel {id}     - Delete a feel by id.\n");

	fprintf(out, "\nJournaling Commands\n");
	fprintf(out, "\tmemo {memo}          - Add a memo.\n");
	fprintf(out, "\tdelete-memo {memo-id}- Delete a memo by id.\n");

	fprintf(out, "\nMetadata Commands\n");
	fprintf(out, "\tdescribe-feel {feel} - Describe a feel.\n");
	fprintf(out, "\tcreate-emotion       - Create a new emotion.\n");
	fprintf(out, "\tcount-feels          - Return a count of feels.\n");
	fprintf(out, "\tcreate-context       - Create a new feels context database.\n");

	fprintf(out, "\nImport/Export Commands\n");
	fprintf(out, "\texport-json          - Dump feels in json format.\n");
	fprintf(out, "\n");
	fprintf(out, "\thelp                 - Print this message.\n");
	fprintf(out, "\tversion              - Print hif version information.\n");
	fprintf(out, "\n");
	fprintf(out, "Examples:\n");
	fprintf(out, "\n");
	fprintf(out, "Creating a new emotion:\n");
	fprintf(out, "$ hif create-emotion \"love\" \"I love you\"\n");

	fprintf(out, "\n");
	fprintf(out, "Journaling a happy feel:\n");
	fprintf(out, "$ hif +love\n");

	
}

static char * str_lower(char * s) {
	char * p = s;
	for (; *p; ++p) { *p = tolower(*p); }
	return s;
}

static hif_command str_to_command(const char * s) {
	const size_t MAX_COMMAND_LENGTH = sizeof("describe-feel") - 1;

	size_t len = strnlen(s, MAX_COMMAND_LENGTH);
	if(*s == '+' || strncmp(s, "add", len) == 0) {
		return HIF_COMMAND_ADD_FEEL;
	} else if(strncmp(s, "describe-feel", len) == 0) {
		return HIF_COMMAND_GET_FEEL_DESCRIPTION;
	} else if(strncmp(s, "export-json", len) == 0) {
		return HIF_COMMAND_JSON;
	} else if(strncmp(s, "delete-feel", len) == 0) {
		return HIF_COMMAND_DELETE_FEEL;
	} else if(strncmp(s, "count-feels", len) == 0) {
		return HIF_COMMAND_COUNT_FEELS;
	} else if(strncmp(s, "create-context", len) == 0) {
		return HIF_COMMAND_CREATE;
	} else if(strncmp(s, "create-emotion", len) == 0) {
		return HIF_COMMAND_CREATE_FEEL;
	} else if(strncmp(s, "memo", len) == 0) {
		return HIF_COMMAND_ADD_MEMO;
	} else if(strncmp(s, "delete-memo", len) == 0) {
		return HIF_COMMAND_DELETE_MEMO;
	} else if(strncmp(s, "help", len) == 0) {
		print_help(stdout);
		exit(0);
	} else if(strncmp(s, "version", len) == 0) {
		print_version(stdout);
		exit(0);
	} else {
		print_help(stderr);
		exit(-1);
	}
}

static void terminate() {
	char const * config_path = get_config_path();
	free((void *)config_path);

	sqlite3_shutdown();
}

static int initialize() {
	int ret = atexit(terminate);
	sqlite3_initialize();
	ensure_config_path();
	
	return ret;
}

static void command_create(storage_interface const * adapter, int argc, char **argv) {
	const char * path = NULL;
	if(argc >= 3) {
		path = argv[2];
	}
	adapter->create_storage(adapter, path);
	fprintf(stdout, "Created context %s\n", path);
}

static void command_count_feels(storage_interface const * adapter, int argc, char **argv) {
	(void)argc;
	(void)argv;

	int count = adapter->count_feels(adapter);
	fprintf(stdout, "%i\n", count);
}

static void command_add_feel(storage_interface const * adapter, int argc, char **argv) {
	if(argc < 2) {
		print_help(stderr);
		exit(-1);
	}

	char * feel = argv[1];
	if(*feel == '+') {
		feel++;
	} else {
		if(argc < 3) {
			print_help(stderr);
			exit(-1);
		}
		feel = argv[2];
	}
	
	char * description = NULL;
	int rc = adapter->insert_feel(adapter, feel, &description);
	if(!rc) {
		fprintf(stderr, "I'm not familiar with the feels '%s'. Try create-emotion, first.\n", feel);
	} else {
		fprintf(stdout, "Feels '%s' logged, %s\n", feel, description ? description : "(yay)");
	}
	
	if(description) free(description), description = NULL;
}

static void command_export(storage_interface const * adapter, int argc, char **argv) {
	(void)argc; (void)argv;
	adapter->export(adapter, NULL);
}

static void command_delete_feel(storage_interface const * adapter, int argc, char **argv) {
	if(argc < 3) {
		print_help(stderr);
		exit(1);
	}
	int id = atoi(argv[2]);
	int affected_rows = 0;
	int rc = adapter->delete_feel(adapter, id, &affected_rows);

	if(!rc || affected_rows == 0) {
		fprintf(stderr, "Nothing to delete, feel id %i is not present!\n", (int)id);
	} else {
		fprintf(stdout, "Feel deleted!\n");
	}
}

static void command_create_feel(storage_interface const * adapter, int argc, char **argv) {
	if(argc < 3) {
		print_help(stderr);
		exit(-1);
	}
	char const * name = argv[2];
	char const * description = NULL;
	if(argc > 3) {
		description = argv[3];	
	}

	int rc = adapter->create_feel(adapter, name, description);
	if(!rc) {
		fprintf(stderr, "Skipped adding feel '%s'; it already exists!\n", name);
	} else {
		fprintf(stdout, "Created feel '%s': %s\n", name, description ? description : "[empty]");
	}
}

static void command_count_memos(storage_interface const * adapter, int argc, char **argv) {
	(void)adapter; (void)argc; (void)argv;
}

static void command_add_memo(storage_interface const * adapter, int argc, char **argv) {
	if(argc < 3) {
		print_help(stderr);
		exit(-1);
	}

	char const * memo = argv[2];

	int affected_rows;
	int rc = adapter->insert_memo(adapter, memo, &affected_rows);

	if(!rc) {
		fprintf(stderr, "Failed to add memo.\n");
	} else {
		fprintf(stdout, "Added new memo, '%s'\n", memo);
	}
}

static void command_delete_memo(storage_interface const * adapter, int argc, char **argv) {
	if(argc < 3) {
		print_help(stderr);
		exit(1);
	}
	int id = atoi(argv[2]);
	int affected_rows = 0;
	int rc = adapter->delete_memo(adapter, id, &affected_rows);

	if(!rc || affected_rows == 0) {
		fprintf(stderr, "Nothing to delete, memo id %i is not present!\n", (int)id);
	} else {
		fprintf(stdout, "Memo deleted!\n");
	}

}

static void command_get_feel_description(storage_interface const * adapter, int argc, char **argv) {
	if(argc < 3) {
		print_help(stderr);
		exit(-1);
	}

	char * feel = argv[2];
	char * description = NULL;

	int rc = adapter->get_feel_description(adapter, feel, &description);
	if(rc) {
		fprintf(stderr, "Feel not found; try list-feels\n");
		exit(-1);
	}

	fprintf(stdout, "%s\n", description);

	free(description), description = NULL;
}

typedef void (*command_fn)(storage_interface const * adapter, int argc, char **argv);

static command_fn fns[] = {
	&command_create, /* HIF_COMMAND_CREATE */
	&command_count_feels, /* HIF_COMMAND_COUNT_FEELS */
	&command_export, /* HIF_COMMAND_JSON */
	&command_delete_feel, /* HIF_COMMAND_DELETE_FEEL */
	&command_create_feel, /* HIF_COMMAND_CREATE_FEEL */
	&command_add_feel, /* HIF_COMMAND_ADD_FEEL */
	&command_count_memos, /* HIF_COMMAND_COUNT_MEMOS */
	&command_add_memo, /* HIF_COMMAND_ADD_MEMO */
	&command_get_feel_description, /* HIF_COMMAND_GET_FEEL_DESCRIPTION */
	&command_delete_memo /* HIF_COMMAND_DELETE_MEMO */
};

int main(int argc, char **argv) {
	static const char * const DB = "hif.db";
	
	int call_terminate_on_exit = initialize();
	if(argc < 2) {
		print_help(stderr);
		exit(-1);
	}

	int ret = -1;
	storage_interface const * adapter = NULL;
	memo_repository_interface const * repository = NULL;

	repository = memo_repository_alloc();
	repository->free((memo_repository_interface*)repository);

	char *p = str_lower(argv[1]);
	
	hif_command command = str_to_command(p);
	if((command < HIF_COMMAND_CREATE) || command > HIF_COMMAND_EOF) goto err0;

	adapter = storage_adapter_alloc();

	if(!context_exists(DB)) {
		ret = adapter->create_storage(adapter, DB);
		if(ret) goto err0;
	}

	ret = adapter->open_storage(adapter, DB);
	if(ret) goto err0;

	fns[command](adapter, argc, argv);
	
	ret = adapter->close(adapter);
	if(ret) goto err0;

	ret = 0;

err0:
	if(adapter) adapter->free(adapter);
	if(call_terminate_on_exit) terminate();
	
	return ret;
}
