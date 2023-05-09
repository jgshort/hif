#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#include "hif.h"
#include "environment.h"
#include "storage_adapter.h"

struct storage_adapter_data;

typedef struct storage_adapter {
	storage_interface _interface;
	
	struct storage_adapter_data * data;
} storage_adapter;

static int create_storage(storage_interface const * adapter, char const * path);
static int open_storage(storage_interface const * adapter, char const * context_name);
static int close(storage_interface const * adapter);

static int create_feel(storage_interface const * adapter, char const * feel, char const * description);
static int get_feel_description(storage_interface const * adapter,  char const * feel, char **description);

static int insert_feel(storage_interface const * adapter, char const * feel, char **description);
static int delete_feel(storage_interface const * adapter, int id, int * affected_rows);
static int count_feels(storage_interface const * adapter);

static int insert_memo(storage_interface const * adapter, char const * memo, int * affected_rows);
static int delete_memo(storage_interface const * adapter, int id, int * affected_rows);

static int export(storage_interface const * adapter, kvp_handler kvp);

static int get_escape_character_count(unsigned char const * source, size_t source_len);

static int delete_from_table_by_id(storage_interface const * adapter, char const * table_name,  int id, int * affected_rows);

typedef struct storage_adapter_data {
	sqlite3 *db;

	int is_open;
} storage_adapter_data;

storage_interface const * storage_adapter_alloc() {
	storage_adapter * adapter = malloc(sizeof * adapter);

	return storage_adapter_init((storage_interface *)adapter);
}

storage_interface const * storage_adapter_init(storage_interface * adapter) {
	storage_adapter_data * data = malloc(sizeof * data);
	((storage_adapter *)adapter)->data = data;

	adapter->create_storage = &create_storage;
	adapter->open_storage = &open_storage;
	adapter->close = &close;
	adapter->free = &storage_adapter_free;

	adapter->create_feel = &create_feel;
	adapter->get_feel_description = &get_feel_description;

	adapter->insert_feel = &insert_feel;
	adapter->delete_feel = &delete_feel;
	adapter->count_feels = &count_feels;
	
	adapter->insert_memo = &insert_memo;
	adapter->delete_memo = &delete_memo;

	adapter->export = &export;

	return adapter;
}

void storage_adapter_free(storage_interface const * adapter) {
	if(!adapter) return; 
	
	adapter->close(adapter);
	if(((storage_adapter *)adapter)->data) {
		free(((storage_adapter *)adapter)->data), ((storage_adapter *)adapter)->data = NULL;
	}
	free((storage_adapter *)adapter), adapter = NULL;
}

static int create_storage(storage_interface const * adapter, char const * context_name) {
	char * err_msg = NULL;

	(void)adapter;

	if(!context_name) context_name = "hif.db";

	char const * config_path = get_config_path();
	char * path = alloc_concat_path(config_path, context_name);

	char * sql = NULL;
	sqlite3 * db = NULL;
	int rc = sqlite3_open(path, &db);
	if(rc != SQLITE_OK) {
		(void)db;
		goto err0;
	}

	asprintf(&sql, 
		"pragma encoding = utf8;" \
		"create table if not exists hif_statuses ("\
			"status_id integer primary key, status text unique not null, description text"\
		"); " \
		"create index if not exists hif_statuses_status_inx on hif_statuses(status); " \
		\
		"create table if not exists hif_feels ("\
			"feel_id integer primary key, feel int, dtm int not null, foreign key (feel) references hif_statuses(status_id)"\
		"); " \
		"create index if not exists hif_feels_feel_inx on hif_feels(feel); " \
		\
		"insert into hif_statuses (status, description) values ('sad', ':(');" \
		"insert into hif_statuses (status, description) values ('meh', ':|');" \
		"insert into hif_statuses (status, description) values ('tired', '😫');" \
		"insert into hif_statuses (status, description) values ('anxious', '😰');" \
		"insert into hif_statuses (status, description) values ('woo', ':)');" \
		"insert into hif_statuses (status, description) values ('shrug', '🤷 ¯\\_(ツ)_/¯');" \
		\
		"create table if not exists hif_contexts (" \
			"context_id integer primary key, name text not null, path text not null" \
		");" \
		"create index if not exists hif_contexts_name_inx on hif_contexts(name);" \
		"insert into hif_contexts (name, path) values ('%s', '%s');" \
		\
		"create table if not exists hif_memos (" \
			"memo_id integer primary key, memo text not null, dtm int not null" \
		");"
		, context_name, path);

	rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);	
	if(rc != SQLITE_OK) goto err1;

	goto err0;

err1:
	fprintf(stderr, "Failed to create context %s, '%s'\n", context_name, err_msg);
err0:
	sqlite3_close(db);
	free(path), path = NULL;

	if(sql) {
		free(sql), sql = NULL;
	}
	
	return rc;
}

static int open_storage(storage_interface const * adapter, char const * context_name) {
	if(!context_name) context_name = "hif.db";

	char * path = alloc_concat_path(get_config_path(), context_name);

	int rc = sqlite3_open(path, &((storage_adapter *)adapter)->data->db);
	if(rc != SQLITE_OK) goto err0;

err0:
	free(path), path = NULL;
	return rc;
}

static int close(storage_interface const * adapter) {
	int ret = -1;
	if(adapter && ((storage_adapter *)adapter)->data) {
		storage_adapter_data * data = ((storage_adapter *)adapter)->data;
		if(data->is_open) {
			sqlite3 *db = data->db;
			if(db) {
				ret = sqlite3_close(db);
			}
			data->is_open = 0;
		}
	}	

	return ret == SQLITE_OK;;
}

static int get_feel_description(storage_interface const * adapter,  char const * feel, char **description) {
	char const * sql = "select description from hif_statuses where status = ?;";

	sqlite3_stmt * stmt = NULL;
	int rc = sqlite3_prepare_v2(((storage_adapter *)adapter)->data->db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK) goto err0;

	rc = sqlite3_bind_text(stmt, 1, feel, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) goto err1;

	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW) {
		char const * desc = (char const *)sqlite3_column_text(stmt, 0);
		if(desc) *description = strdup(desc);
		rc = SQLITE_OK;
	}

err1:
	sqlite3_finalize(stmt);

err0:
	return rc;
}

static int insert_feel(storage_interface const * adapter, char const * feel, char **description) {
	char const * sql = "insert into hif_feels (feel, dtm) values (" \
		"(select status_id from hif_statuses where status = ?), datetime('now')" \
		");";
	
	if(!feel) return -1;
	
	sqlite3_stmt * stmt = NULL;
	int rc = sqlite3_prepare_v2(((storage_adapter *)adapter)->data->db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK) goto err0;

	rc = sqlite3_bind_text(stmt, 1, feel, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) goto err1;

	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) goto err1;

	rc = adapter->get_feel_description(adapter, feel, description);

err1:	
	sqlite3_finalize(stmt), stmt = NULL;
err0:

	return rc == SQLITE_OK;
}

static int create_feel(storage_interface const * adapter, char const * feel, char const * description) {
	char const * sql = "insert into hif_statuses (status, description) values (?, ?);";
	
	if(!feel) return -1;
	
	sqlite3_stmt * stmt = NULL;
	int rc = sqlite3_prepare_v2(((storage_adapter *)adapter)->data->db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK) goto err0;

	rc = sqlite3_bind_text(stmt, 1, feel, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) goto err1;

	rc = sqlite3_bind_text(stmt, 2, description, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) goto err1;

	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) goto err1;

	rc = SQLITE_OK;

err1:
	sqlite3_finalize(stmt), stmt = NULL;
err0:

	return rc == SQLITE_OK;
}

static int delete_memo(storage_interface const * adapter, int id, int * affected_rows) {
	return delete_from_table_by_id(adapter, "hif_memos", id, affected_rows);
}

static int delete_feel(storage_interface const * adapter, int id, int * affected_rows) {
	return delete_from_table_by_id(adapter, "hif_feels", id, affected_rows);
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
	free(sql), sql = NULL;

	return count;
}

static int count_feels(storage_interface const * adapter) {
	int count = query_table_row_count(((storage_adapter *)adapter)->data->db, "hif_feels");
	return count;
}

static int to_kvp(char const * key, char const * value, int is_numeric) {
	fprintf(stdout, "\"%s\": ", key);
	if(is_numeric) {
		fprintf(stdout, "%s", value);
	} else {
		fprintf(stdout, "\"%s\"", value);
	}

	return 0;
}

static int get_escape_character_count(unsigned char const * source, size_t source_len) {
	size_t len = source_len;

	unsigned char const * c = source;
	do {
		switch(*c) {
			case '\\':
			case '"':
			case '\b':
			case '\t':
			case '\n':
			case '\f':
			case '\r':
				len += 1;
				break;
			default:
				if(*c <= 32) {
					len += sizeof("\\u0000") - 1;
				}
				break;
		}
	} while(++c && *c);

	return len;
}

static char * alloc_json_escape_string(unsigned char const * source) {
	if(!source) return NULL;

	size_t source_len = strlen((char const *)source) + 1;
	size_t len = get_escape_character_count(source, source_len);
	
	char * dest = malloc(len);
	if(!dest) return NULL;

	if(len == source_len) {
		memcpy(dest, source, source_len);
		dest[source_len - 1] = 0;
		return dest;
	}

	char * d = dest;
	unsigned char const * c = source;
	do {
		switch(*c) {
			case '\\':
			case '"':
			case '\b':
			case '\t':
			case '\n':
			case '\f':
			case '\r':
				*d = '\\'; ++d;
				*d = *c;
				break;
			default:
				if((*c <= 31) || (*c == '\"') || (*c == '\\')) {
					*d = '\\'; ++d; *d = 'u'; ++d;
					sprintf(d, "%04x", *c);
                    			d += 4;
				} else {
					*d = *c;
				}
				break;
		}
	} while(++d, ++c && *c);
	*d = 0;

	return dest;
}

static int export(storage_interface const * adapter, kvp_handler kvp) {
	int count = count_feels(adapter);
	if(count < 0) { exit(-1); }

	sqlite3_stmt * stmt = NULL;
	sqlite3 *db = ((storage_adapter *)adapter)->data->db;

	if(!kvp) kvp = &to_kvp;

	char * sql = "select f.feel_id 'id', s.status 'feel', s.description 'description', f.dtm 'datetime' "\
		      "from hif_feels f inner join hif_statuses s on f.feel = s.status_id;";

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
			unsigned char const * key = (unsigned char const *)sqlite3_column_name(stmt, col);
			unsigned char const * value = (unsigned char const *)sqlite3_column_text(stmt, col);

			char * escaped_key = alloc_json_escape_string(key);
			char * escaped_value = alloc_json_escape_string(value);
			int type = sqlite3_column_type(stmt, col);

			kvp(escaped_key, escaped_value, (type == SQLITE_INTEGER || type == SQLITE_FLOAT));
		
			free(escaped_key), escaped_key = NULL;
			free(escaped_value), escaped_value = NULL;
			if(col < col_count - 1) fprintf(stdout, ", ");
			col++;
		}
		i++;
		if(i < count - 1) fprintf(stdout, " },\n");
		else fprintf(stdout, " }\n");

	}

	sqlite3_finalize(stmt);
	if(count > 0) fprintf(stdout, "\t");
	fprintf(stdout, "]\n");
	fprintf(stdout, "}\n");

err2:
	return rc;
}

static int insert_memo(storage_interface const * adapter, char const * memo, int * affected_rows) {
	char const * sql = "insert into hif_memos (memo, dtm) values (?, datetime('now'));";

	sqlite3_stmt * stmt = NULL;

	int rc = sqlite3_prepare_v2(((storage_adapter *)adapter)->data->db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK) goto err0;

	rc = sqlite3_bind_text(stmt, 1, memo, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) goto err1;

	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) goto err1;

	*affected_rows = sqlite3_changes(((storage_adapter *)adapter)->data->db);

	rc = SQLITE_OK;
err1:
	sqlite3_finalize(stmt);

err0:
	return rc == SQLITE_OK;
}

static int is_table_name_valid(char const * table_name) {
	/* Assumes *ACCEPTABLE_TABLES is max strlen() of ACCEPTABLE_TABLES[] */
	const char * ACCEPTABLE_TABLES[] = {"hif_feels", "hif_memos" };
	const size_t ACCEPTABLE_TABLES_LEN = sizeof(ACCEPTABLE_TABLES) / sizeof(*ACCEPTABLE_TABLES);
	const size_t MAX_TABLE_NAME_LEN = sizeof(*ACCEPTABLE_TABLES) - 1;

	if(!table_name) return false;

	bool found = false;
	for(size_t i = 0; i < ACCEPTABLE_TABLES_LEN; i++) {
		if(strncmp(table_name, ACCEPTABLE_TABLES[i], MAX_TABLE_NAME_LEN) == 0) {
			found = true;
			break;
		}
	}

	return found;
}

static int delete_from_table_by_id(storage_interface const * adapter, char const * table_name,  int id, int * affected_rows) {
	int found = is_table_name_valid(table_name);
	if(!found) abort();

	int rc = -1;

	char * sql = NULL;
	asprintf(&sql, "delete from %s where rowid = ?;", table_name);
	if(!sql) goto err0;

	sqlite3_stmt * stmt = NULL;
	rc = sqlite3_prepare_v2(((storage_adapter *)adapter)->data->db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK) goto err1;

	rc = sqlite3_bind_int(stmt, 1, id);
	if(rc != SQLITE_OK) goto err2;

	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) goto err2;

	*affected_rows = sqlite3_changes(((storage_adapter *)adapter)->data->db);

	rc = SQLITE_OK;

err2:
	sqlite3_finalize(stmt);

err1:
	free(sql), sql = NULL;

err0:
	return rc == SQLITE_OK;
}

