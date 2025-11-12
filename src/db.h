#pragma once

#include <sqlite3.h>
#include <pthread.h>

typedef struct {
    sqlite3 *db;
    pthread_mutex_t lock;
} DB;

int db_open(DB *d, const char *path);
int db_close(DB *d);
int db_init_schema(DB *d);
int db_insert_log(DB *d, const char *source, const char *unit, const char *message, const char *ts);
// Search with pagination: limit and offset. If query is NULL or empty, returns recent logs.
int db_search(DB *d, const char *query, int limit, int offset, sqlite3_stmt **out_stmt);
// Fetch full message text for a given log id. Caller receives a newly allocated string and must free it.
int db_get_message(DB *d, int log_id, char **out_message);
// Tagging APIs
int db_init_tags(DB *d);
int db_add_tag(DB *d, int log_id, const char *tag);
int db_remove_tag(DB *d, int log_id, const char *tag);
// Returns a newly allocated char** array terminated by NULL; caller frees with db_free_string_array
char **db_list_tags(DB *d, int log_id);
void db_free_string_array(char **arr);

