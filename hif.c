#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

#define HIF_EXECUTABLE "hif"
#define HIF_VERSION "v1.0.0"

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

typedef struct hif_init_payload {
	sqlite3 **db;
	char * path;
} hif_init_payload;

static void print_version(FILE * out) {
	fprintf(out, HIF_EXECUTABLE " " HIF_VERSION "\n");
}

static void print_help(FILE * out) {
	print_version(out);	
	fprintf(out, "usage: hif [mood | command (args)*]\n\n");
	if(out == stderr) {
		fprintf(out, "Sorry bud, you need to tell me how you feel.\n");
		fprintf(out, "I support the following moods:\n");
		fprintf(out, "\tbad, meh, woo\n\n");
		fprintf(out, "Example:\n");
		fprintf(out, "\t$ hif woo\n");
		fprintf(out, "\nOr try a command:\n");
	}
	fprintf(out, "\tcount        - Return a count of feels.\n");
	fprintf(out, "\tjson         - Dump feels in json format.\n");
	fprintf(out, "\tdelete {id}  - Delete feel by provided id, i.e:\n\t\t$ hif delete 1\n\n");
	fprintf(out, "\thelp         - Print this message.\n");
	fprintf(out, "\tversion      - Print hif version information.\n");
	fprintf(out, "\n");
}

static int create_table_fn(void * data, int argc, char **argv, char **col) {
	return 0;
}

static int insert_fn(void *data, int argc, char **argv, char **col) {
	return 0;
}

static int delete_fn(void *data, int argc, char **argv, char **col) {
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

static int insert_command(sqlite3 * db, void * payload) {
	char * sql = NULL;
	int rc = SQLITE_OK;
	char * err_msg;

	asprintf(&sql, "insert into hif_feels (feel, dtm) values (%i, datetime('now'));", (int)payload);
	rc = sqlite3_exec(db, sql, &insert_fn, 0, &err_msg);	

	free(sql);

	return rc;
}

static int query_table_row_count(sqlite3 * db, const char * table_name) {
	int count = -1;
	char * sql = NULL;
	sqlite3_stmt * stmt = NULL;

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

static int init_command(sqlite3 * db, void * payload) {
	char * err_msg = NULL;
	hif_init_payload * init_payload = (hif_init_payload*)payload;

	if(!init_payload->path) init_payload->path = "hif.db";

	int rc = sqlite3_open(init_payload->path, init_payload->db);
	if(rc != SQLITE_OK) goto err0;

	char * sql = "create table if not exists hif_feels (feel_id integer primary key, feel int, dtm int);" \
		"create index if not exists hif_feels_feel_inx on hif_feels(feel);";

	rc = sqlite3_exec(db, sql, &create_table_fn, 0, &err_msg);	
	if(rc != SQLITE_OK) goto err1;

	goto err0;
err2:
err1:
err0:
	return rc;
}

static int count_command(sqlite3 * db, void * payload) {
	payload = payload;
	int count = query_table_row_count(db, "hif_feels");
	fprintf(stdout, "%i\n", count);	
	return count;
}

static int delete_command(sqlite3 * db, void * payload) {
	char * err_msg = NULL;
	char * sql = NULL;

	int id = (int)payload;
	asprintf(&sql, 
		"delete from hif_feels where feel_id = %i; " \
		" select feel_id from hif_feels where feel_id = %i;", id, id);
	int rc = sqlite3_exec(db, sql, &delete_fn, 0, &err_msg);
	
	if(rc != SQLITE_OK) {
		fprintf(stderr, "Feel %i not deleted.\n", (int)id);
	} else {
		fprintf(stdout, "Feel deleted!\n");
	}

	free(sql);

	return rc;
}

static int query_command(sqlite3 * db, void * payload) {
	sqlite3_stmt * stmt = NULL;
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
			const char * data = NULL;
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
	if(strncmp(s, "meh", sizeof("meh") - 1) == 0) {
		command.feel = HIF_MEH;
	} else if(strncmp(s, "woo", sizeof("woo") - 1) == 0) {
		command.feel = HIF_WOO;
	} else if(strncmp(s, "bad", sizeof("bad") - 1) == 0) {
		command.feel = HIF_BAD;
	} else if(strncmp(s, "json", sizeof("json") - 1) == 0) {
		command.feel = HIF_COMMAND;
		command.command = &query_command;
	} else if(strncmp(s, "delete", sizeof("delete") - 1) == 0) {
		command.feel = HIF_COMMAND;
		command.command = &delete_command;
	} else if(strncmp(s, "count", sizeof("count") - 1) == 0) {
		command.feel = HIF_COMMAND;
		command.command = &count_command;
	} else if(strncmp(s, "help", sizeof("help") - 1) == 0) {
		print_help(stdout);
		exit(0);
	} else if(strncmp(s, "init", sizeof("init") - 1) == 0) {
		command.feel = HIF_COMMAND;
		command.command = &init_command;
	} else if(strncmp(s, "version", 7) == 0) {
		print_version(stdout);
		exit(0);
	} else {
		print_help(stderr);
		exit(-1);
	}

	return command;
}

static const char * get_user_home() {
	const char * env_home = getenv("HOME");
	if(!env_home) {
		struct passwd *pw = getpwuid(getuid());
		env_home = pw->pw_dir;
	}
	return env_home;
}

static void ensure_config_path() {
	char * config_path = NULL;
	const char * home = get_user_home();

	asprintf(&config_path, "%s/.config/hif", home);
	
	free(config_path);
	struct stat st = {0};
	if (stat(config_path, &st) == -1) {
		mkdir(config_path, 0700);
	}
}

int main(int argc, char **argv) {
	sqlite3 * db = NULL;
	char * err_msg = NULL, * sql = NULL;
	int rc = 0, ret = -1;
	hif_command command;

	ensure_config_path();

	if(argc < 2) {
		print_help(stderr);
		goto err0;
	}

	char *p = str_lower(argv[1]);

	sqlite3_initialize();

	command = str_to_command(p);
	hif_feel feel = command.feel;

	assert((feel >= HIF_BAD && feel <= HIF_WOO) || (feel == HIF_COMMAND));

	if(command.command == &init_command) {
		hif_init_payload payload;
		payload.db = &db;
		payload.path = NULL;
		if(argc >= 3) {
			payload.path = argv[2];
		}
		rc = command.command(db, &payload);
	}
	else if(command.command == &insert_command) {
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
			print_help(stderr);
			goto err1;
		}
		int id = atoi(argv[2]);
		rc = command.command(db, (void*)(long)id);
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
