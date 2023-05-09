#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#include "hif.h"
#include "environment.h"
#include "storage_adapter.h"

static int create_storage(storage_adapter const * adapter, char const * path);
static int open_storage(storage_adapter const * adapter, char const * context_name);
static void close(storage_adapter const * adapter);

static int create_feel(storage_adapter const * adapter, char const * feel, char const * description);

static int insert_feel(storage_adapter const * adapter, char const * feel, char **description);
static int delete_feel(storage_adapter const * adapter, int id);
static int count_feels(storage_adapter const * adapter);

static int insert_memo(storage_adapter const * adapter, char const * memo);

static int export(storage_adapter const * adapter, kvp_handler kvp);

static int create_table_fn(void * data, int argc, char **argv, char **col);
static int insert_fn(void *data, int argc, char **argv, char **col);

typedef struct storage_adapter_data {
	sqlite3 *db;

	int is_open;
} storage_adapter_data;

storage_adapter const * storage_adapter_alloc() {
	storage_adapter * adapter = malloc(sizeof * adapter);

	return storage_adapter_init(adapter);
}

storage_adapter const * storage_adapter_init(storage_adapter * adapter) {
	storage_adapter_data * data = malloc(sizeof * data);
	adapter->data = data;

	adapter->create_storage = &create_storage;
	adapter->open_storage = &open_storage;
	adapter->close = &close;
	adapter->free = &storage_adapter_free;

	adapter->create_feel = &create_feel;
	adapter->insert_feel = &insert_feel;
	adapter->delete_feel = &delete_feel;
	adapter->count_feels = &count_feels;
	
	adapter->insert_memo = &insert_memo;

	adapter->export = &export;

	return adapter;
}

void storage_adapter_free(storage_adapter * adapter) {
	if(adapter) {
		adapter->close(adapter);
		if(adapter->data) {
			free(adapter->data), adapter->data = NULL;
		}
		free(adapter), adapter = NULL;
	}
}

static int create_storage(storage_adapter const * adapter, char const * context_name) {
	char * err_msg = NULL;

	if(!context_name) context_name = "hif.db";

	char const * config_path = get_config_path();
	char * path = alloc_concat_path(config_path, context_name);

	sqlite3 * db = NULL;
	int rc = sqlite3_open(path, &db);
	if(rc != SQLITE_OK) goto err0;

	char * sql = NULL;
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
		"insert into hif_statuses (status, description) values ('bad', ':(');" \
		"insert into hif_statuses (status, description) values ('meh', ':|');" \
		"insert into hif_statuses (status, description) values ('woo', ':)');" \
		"insert into hif_statuses (status, description) values ('shrug', '🤷 Shrug ¯\\_(ツ)_/¯ Emoji');" \
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

	rc = sqlite3_exec(db, sql, &create_table_fn, 0, &err_msg);	
	if(rc != SQLITE_OK) goto err1;

	goto err0;
err2:
err1:
	fprintf(stderr, "Failed to create context %s, '%s'\n", context_name, err_msg);
err0:
	sqlite3_close(db);
	free(path), path = NULL;
	free(sql), sql = NULL;
	
	return rc;
}

static int open_storage(storage_adapter const * adapter, char const * context_name) {
	if(!context_name) context_name = "hif.db";

	char * path = alloc_concat_path(get_config_path(), context_name);

	int rc = sqlite3_open(path, &adapter->data->db);
	if(rc != SQLITE_OK) goto err0;

err0:
	free(path), path = NULL;
	return rc;
}

static void close(storage_adapter const * adapter) {
	if(adapter && adapter->data) {
		storage_adapter_data * data = adapter->data;
		if(data->is_open) {
			sqlite3 *db = data->db;
			if(db) {
				sqlite3_close(db);
			}
			data->is_open = 0;
		}
	}	
}

static int create_table_fn(void * data, int argc, char **argv, char **col) {
	return 0;
}

static int insert_fn(void *data, int argc, char **argv, char **col) {
	return 0;
}

/*
static int select_description(void *data, int argc, char **argv, char **col) {
	fprintf(stdout, "%s\n", argv[0]);
	return 0;
} */

static int insert_feel(storage_adapter const * adapter, char const * feel, char **description) {
	char * sql = NULL;
	char * err_msg = NULL;
	if(!feel) return -1;
	
	sqlite3_stmt * stmt = NULL;

	asprintf(&sql, "insert into hif_feels (feel, dtm) values (" \
		"(select status_id from hif_statuses where status = '%s'), datetime('now')" \
		");", feel);
	int rc = sqlite3_exec(adapter->data->db, sql, &insert_fn, 0, &err_msg);

	asprintf(&sql, "select description from hif_statuses where status = '%s';", feel);

	rc = sqlite3_prepare_v2(adapter->data->db, sql, -1, &stmt, NULL);
	if(rc != SQLITE_OK) goto err0;

	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW) {
		char const * desc = (char const *)sqlite3_column_text(stmt, 0);
		*description = strdup(desc);
		rc = SQLITE_OK;
	} 

	sqlite3_finalize(stmt);

err0:
	free(sql), sql = NULL;

	return rc == SQLITE_OK;
}

static int create_feel(storage_adapter const * adapter, char const * feel, char const * description) {
	char * sql = NULL;
	char * err_msg;
	char * wrapped_description = NULL;

	if(!feel) {
		return -1;
	}

	if(!description) {
		asprintf(&wrapped_description, "%s", "NULL");
	}
	else {
		asprintf(&wrapped_description, "'%s'", description);
	}
	asprintf(&sql, "insert into hif_statuses (status, description) values ('%s', %s);", feel, wrapped_description);
	int rc = sqlite3_exec(adapter->data->db, sql, &insert_fn, 0, &err_msg);

	free(wrapped_description), wrapped_description = NULL;
	free(sql), sql = NULL;

	return rc == SQLITE_OK;
}

static int delete_fn(void *data, int argc, char **argv, char **col) {
	return 0;
}

static int delete_feel(storage_adapter const * adapter, int id) {
	char * err_msg = NULL;
	char * sql = NULL;

	asprintf(&sql, 
		"delete from hif_feels where feel_id = %i; " \
		" select feel_id from hif_feels where feel_id = %i;", id, id);

	sqlite3 *db = adapter->data->db;
	int rc = sqlite3_exec(db, sql, &delete_fn, 0, &err_msg);
	
	free(sql), sql = NULL;
	return rc == SQLITE_OK;
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

static int count_feels(storage_adapter const * adapter) {
	int count = query_table_row_count(adapter->data->db, "hif_feels");
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

static char * alloc_json_escape_string(unsigned char const * source) {
	if(!source) return NULL;

	size_t source_len = strlen((char const *)source) + 1;
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

	char * dest = malloc(len);
	if(!dest) return NULL;

	if(len == source_len) {
		memcpy(dest, source, source_len);
		dest[source_len - 1] = 0;
		return dest;
	}

	char * d = dest;
	c = source;
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

static int export(storage_adapter const * adapter, kvp_handler kvp) {
	int count = count_feels(adapter);
	if(count < 0) { exit(-1); }

	sqlite3_stmt * stmt = NULL;
	sqlite3 *db = adapter->data->db;

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
			char const * key = (char const *)sqlite3_column_name(stmt, col);
			unsigned char const * value = (unsigned char const *)sqlite3_column_text(stmt, col);

			char * escaped = alloc_json_escape_string(value);
			int type = sqlite3_column_type(stmt, col);

			kvp(key, escaped, (type == SQLITE_INTEGER || type == SQLITE_FLOAT));
		
			free(escaped), escaped = NULL;
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
err1:
err0:
	return rc;
}

static int insert_memo(storage_adapter const * adapter, char const * memo) {
	char * err_msg = NULL;
	char * sql = NULL;

	asprintf(&sql, "insert into hif_memos (memo, dtm) values ('%s', datetime('now'));", memo);

	int rc = sqlite3_exec(adapter->data->db, sql, &insert_fn, 0, &err_msg);

err0:
	free(sql), sql = NULL;

	return rc;
}

