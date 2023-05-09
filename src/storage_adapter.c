#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>

#include "hif.h"
#include "environment.h"
#include "storage_adapter.h"

static int create_storage(storage_adapter const * adapter, char const * path);
static int open_storage(storage_adapter const * adapter, char const * context_name);
static void close(storage_adapter const * adapter);

static int create_feel(storage_adapter const * adapter, char const * feel, char const * description);

static int insert_feel(storage_adapter const * adapter, hif_feel feel);
static int delete_feel(storage_adapter const * adapter, int id);
static int count_feels(storage_adapter const * adapter);

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

	char * sql = 
		"create table if not exists hif_statuses ("\
			"status_id integer primary key, status text not null, description text"\
		"); " \
		"create index if not exists hif_statuses_status_inx on hif_statuses(status); " \
		\
		"create table if not exists hif_feels ("\
			"feel_id integer primary key, feel int, dtm int, foreign key (feel) references hif_statuses(status_id)"\
		"); " \
		"create index if not exists hif_feels_feel_inx on hif_feels(feel); " \
		"insert into hif_statuses (status, description) values ('bad', ':(');" \
		"insert into hif_statuses (status, description) values ('meh', ':|');" \
		"insert into hif_statuses (status, description) values ('woo', ':)');";

	rc = sqlite3_exec(db, sql, &create_table_fn, 0, &err_msg);	
	if(rc != SQLITE_OK) goto err1;

	goto err0;
err2:
err1:
	fprintf(stderr, "Failed to create context %s, '%s'\n", context_name, err_msg);
err0:
	sqlite3_close(db);
	free(path);
	
	return rc;
}

static int open_storage(storage_adapter const * adapter, char const * context_name) {
	if(!context_name) context_name = "hif.db";

	char * path = alloc_concat_path(get_config_path(), context_name);

	int rc = sqlite3_open(path, &adapter->data->db);
	if(rc != SQLITE_OK) goto err0;

err0:
	free(path);
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

static int insert_feel(storage_adapter const * adapter, hif_feel feel) {
	char * sql = NULL;
	char * err_msg;

	asprintf(&sql, "insert into hif_feels (feel, dtm) values (%i, datetime('now'));", (int)feel);
	int rc = sqlite3_exec(adapter->data->db, sql, &insert_fn, 0, &err_msg);

	free(sql);

	return rc;
}

static int create_feel(storage_adapter const * adapter, char const * feel, char const * description) {
	char * sql = NULL;
	char * err_msg;
	char * wrapped_description = NULL;

	if(!feel) {
		return -1;
	}

	if(!description) {
		asprintf(&wrapped_description, "%s", "NULL");;
	}
	else {
		asprintf(&wrapped_description, "'%s'", description);
	}
	asprintf(&sql, "insert into hif_statuses (status, description) values ('%s', %s);", feel, wrapped_description);
	int rc = sqlite3_exec(adapter->data->db, sql, &insert_fn, 0, &err_msg);

	free(wrapped_description);
	free(sql);

	return rc;
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
	
	if(rc != SQLITE_OK) {
		fprintf(stderr, "Feel %i not deleted.\n", (int)id);
	} else {
		fprintf(stdout, "Feel deleted!\n");
	}

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

static int count_feels(storage_adapter const * adapter) {
	int count = query_table_row_count(adapter->data->db, "hif_feels");
	return count;
}
