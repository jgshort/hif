#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

typedef int (*fn_command)(sqlite3 * db, void * payload);

typedef enum hif_feel {
	HIF_BAD,
	HIF_MEH,
	HIF_WOO,

	HIF_COMMAND
} hif_feel;

typedef struct hif_command {
	hif_feel feel;
	fn_command command;
} hif_command;

static void print_help() {
	fprintf(stderr, "Sorry bud, you need to tell me how you feel today.\n");
	fprintf(stderr, "Try one of these:\n");
	fprintf(stderr, "- bad\n- meh\n- woo\n\n");
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "\t$ hif woo\n\n");
	fprintf(stderr, "\nOr try:\n");
	fprintf(stderr, "\tcount        - Return a count of feels.\n");
	fprintf(stderr, "\tjson         - Dump feels in json format.\n");
	fprintf(stderr, "\tdelete {id}  - Delete feel by provided id, i.e:\n\t\t$ hif delete 1\n\n");
	fprintf(stderr, "\thelp         - Print this message.\n");
	fprintf(stderr, "\tversion      - Print hif version information.\n");
	fprintf(stderr, "\n");
}

static int create_table_fn(void *unused, int argc, char **argv, char **col) {
	unused = unused;
	int i;
	for(i = 0; i < argc; i++) {
		fprintf(stdout, "%s = %s\n", col[i], argv[i] ? argv[i] : "NULL");
	}
	fprintf(stdout, "\n");
	return 0;
}

static int insert_fn(void *NotUsed, int argc, char **argv, char **col) {
	int i;
	for(i = 0; i<argc; i++) {
		printf("%s = %s\n", col[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

static int delete_fn(void *NotUsed, int argc, char **argv, char **col) {
	int i;
	for(i = 0; i<argc; i++) {
		printf("%s = %s\n", col[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

static int select_fn(void *data, int argc, char **argv, char **col){
   	int i;
	fprintf(stdout, "\t\t{");
   	for(i = 0; i<argc; i++){
		fprintf(stdout, "'%s': '%s'", col[i], argv[i] ? argv[i] : "NULL");
		if(i < argc - 1) fprintf(stdout, ", ");
   	}
	fprintf(stdout, "}\n");
   	return 0;
}

static char * str_lower(char * s) {
	char *p = s;
	for ( ; *p; ++p) { *p = tolower(*p); }
	return s;
}

static int insert_command(sqlite3 *db, void *payload) {
	char * sql = NULL;
	int rc = SQLITE_OK;
	char * err_msg;

	asprintf(&sql, "insert into hif_feels (feel, dtm) values (%i, datetime('now'));", (int)payload);
	rc = sqlite3_exec(db, sql, &insert_fn, 0, &err_msg);	

	free(sql);

	return rc;
}

static int query_table_row_count(sqlite3 *db, const char * table_name) {
	int count = -1;
	char * sql = NULL;
	sqlite3_stmt *stmt = NULL;

	asprintf(&sql, "select count(*) from %s;", table_name);

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(rc != SQLITE_OK) goto err0;
	
	while(sqlite3_step(stmt) == SQLITE_ROW) {
		count = sqlite3_column_int(stmt, 0);
	}

	sqlite3_finalize(stmt);

err0:
	free(sql);

	return count;
}

static int count_command(sqlite3 * db, void *payload) {
	payload = payload;
	int count = query_table_row_count(db, "hif_feels");
	fprintf(stdout, "%i\n", count);	
	return count;
}

static int delete_command(sqlite3 *db, void *payload) {
	int id = (int)payload;
	char * err_msg = NULL;
	char * sql = NULL;

	asprintf(&sql, "delete from hif_feels where feel_id = %i;", (int)id);
	int rc = sqlite3_exec(db, sql, &delete_fn, 0, &err_msg);

	free(sql);

	return rc;
}

static int query_command(sqlite3 *db, void *payload) {
	sqlite3_stmt *stmt = NULL;
	char * sql = "select * from hif_feels;";
		
	int count = query_table_row_count(db, "hif_feels");
	if(count < 0) { exit(-1); }
	
	fprintf(stdout, "{\n");
	fprintf(stdout, "\t\"feels\": [");
	if(count > 0) fprintf(stdout, "\n");
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK) goto err2;

	int i = 0;
	int col_count = sqlite3_column_count(stmt);

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		int col = 0;

		fprintf(stdout, "\t\t{ ");
		while(col < col_count) {
			const char *data = NULL;
			const int feel_col = 1;

			data = (const char *)sqlite3_column_name(stmt, col);
			fprintf(stdout, "\"%s\": ", data);
			data = (const char *)sqlite3_column_text(stmt, col);
			if(col < 1) {
				fprintf(stdout, "%s", data);
			} else {
				if(col == feel_col) {
					if(data[0] == '0') {
						fprintf(stdout, "\"bad\"");
					} else if(data[0] == '1') {
						fprintf(stdout, "\"meh\"");
					} else if(data[0] == '2') {
						fprintf(stdout, "\"woo\"");
					}
				} else {
					fprintf(stdout, "\"%s\"", data);
				}
			}
			if(col < col_count - 1) fprintf(stdout, ", ");
			col++;
		}
		i++;
		if(i < count) fprintf(stdout, " },\n");
		else fprintf(stdout, " }\n");

	}

	sqlite3_finalize(stmt);
	if(count > 0) fprintf(stdout, "\t");
	fprintf(stdout, "]\n");
	fprintf(stdout, "}\n");

err2:
err1:
err0:
	return rc;
}

static hif_command str_to_command(const char * s) {
	hif_command command;
	
	command.command = &insert_command;
	if(strncmp(s, "meh", 3) == 0) {
		command.feel = HIF_MEH;
	} else if(strncmp(s, "woo", 3) == 0) {
		command.feel = HIF_WOO;
	} else if(strncmp(s, "bad", 3) == 0) {
		command.feel = HIF_BAD;
	} else if(strncmp(s, "json", 5) == 0) {
		command.feel = HIF_COMMAND;
		command.command = &query_command;
	} else if(strncmp(s, "delete", 6) == 0) {
		command.feel = HIF_COMMAND;
		command.command = &delete_command;
	} else if(strncmp(s, "count", 5) == 0) {
		command.feel = HIF_COMMAND;
		command.command = &count_command;
	} else if(strncmp(s, "help", 4) == 0) {
		print_help();
		exit(0);
	} else if(strncmp(s, "version", 7) == 0) {
		fprintf(stdout, "hif v1.0.0\n");
		exit(0);
	} else {
		print_help();
		exit(-1);
	}

	return command;
}

int main(int argc, char **argv) {
	sqlite3 *db = NULL;
	char * err_msg = NULL, *sql = NULL;
	int rc = 0, ret = -1;
	hif_command command;

	if(argc < 2) {
		print_help();
		goto err0;
	}

	char *p = str_lower(argv[1]);

	hif_feel feel;
	command = str_to_command(p);
	feel = command.feel;

	assert((feel >= HIF_BAD && feel <= HIF_WOO) || (feel == HIF_COMMAND));

	
	sqlite3_initialize();
	rc = sqlite3_open("hif.db", &db);
	if(rc) goto err0;

	assert(db);

	sql = "create table if not exists hif_feels (feel_id integer primary key, feel int, dtm int);" \
		"create index if not exists hif_feels_feel_inx on hif_feels(feel);";

	rc = sqlite3_exec(db, sql, &create_table_fn, 0, &err_msg);	
	if(rc != SQLITE_OK) goto err1;

	if(command.command == &insert_command) {
		rc = command.command(db, (void*)feel);
		fprintf(stdout, "Feels added");
		switch(command.feel) {
			case HIF_BAD:
				fprintf(stdout, " :("); break;
			case HIF_MEH:
				fprintf(stdout, " :|"); break;
			case HIF_WOO:
				fprintf(stdout, " :)"); break;
			default:
				fprintf(stdout, " ¯\\_(ツ)_/¯");
		}
		fprintf(stdout, "\n");

	}
	else if(command.command == &query_command) {
		rc = command.command(db, NULL);
	} else if(command.command == &delete_command) {
		if(argc < 3) {
			print_help();
			goto err1;
		}
		int id = atoi(argv[2]);
		rc = command.command(db, (void*)(long)id);
		fprintf(stdout, "Feel removed.\n");
	} else {
		rc = command.command(db, NULL);
	}

	goto err1;

	ret = 0;

err3:
err2:
err1:
	sqlite3_close(db);
	sqlite3_shutdown();

err0:
	return ret;
}
