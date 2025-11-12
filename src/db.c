#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int db_open(DB *d, const char *path) {
    if (sqlite3_open(path, &d->db) != SQLITE_OK) {
        fprintf(stderr, "Failed to open DB: %s\n", sqlite3_errmsg(d->db));
        return -1;
    }
    /* Initialize mutex for protecting sqlite3 access from multiple threads
     * (indexer threads + UI thread). Use default attributes. */
    if (pthread_mutex_init(&d->lock, NULL) != 0) {
        fprintf(stderr, "Failed to init DB mutex\n");
        sqlite3_close(d->db);
        d->db = NULL;
        return -1;
    }
    if (db_init_schema(d) != 0) return -1;
    /* ensure tags tables exist */
    if (db_init_tags(d) != 0) return -1;
    return 0;
}

int db_close(DB *d) {
    if (!d || !d->db) return 0;
    sqlite3_close(d->db);
    d->db = NULL;
    pthread_mutex_destroy(&d->lock);
    return 0;
}

int db_init_schema(DB *d) {
    const char *sql =
        "BEGIN;"
        "CREATE TABLE IF NOT EXISTS logs(id INTEGER PRIMARY KEY, source TEXT, unit TEXT, ts TEXT, message TEXT);"
        "CREATE VIRTUAL TABLE IF NOT EXISTS logs_fts USING fts5(message, content='logs', content_rowid='id');"
        "CREATE TRIGGER IF NOT EXISTS logs_ai AFTER INSERT ON logs BEGIN"
        "  INSERT INTO logs_fts(rowid, message) VALUES(NEW.id, NEW.message);"
        "END;"
        "COMMIT;";

    char *errmsg = NULL;
    if (sqlite3_exec(d->db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "Failed init schema: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    return 0;
}

int db_init_tags(DB *d) {
    const char *sql =
        "BEGIN;"
        "CREATE TABLE IF NOT EXISTS tags(id INTEGER PRIMARY KEY, name TEXT UNIQUE);"
        "CREATE TABLE IF NOT EXISTS log_tags(log_id INTEGER, tag_id INTEGER, UNIQUE(log_id, tag_id));"
        "COMMIT;";
    char *errmsg = NULL;
    if (sqlite3_exec(d->db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "Failed init tags: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    return 0;
}

int db_add_tag(DB *d, int log_id, const char *tag) {
    if (!d || !d->db) return -1;
    pthread_mutex_lock(&d->lock);
    sqlite3_stmt *stmt = NULL;
    const char *ins_tag = "INSERT OR IGNORE INTO tags(name) VALUES(?);";
    if (sqlite3_prepare_v2(d->db, ins_tag, -1, &stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_bind_text(stmt, 1, tag, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_DONE) sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    const char *get_tagid = "SELECT id FROM tags WHERE name=? LIMIT 1;";
    if (sqlite3_prepare_v2(d->db, get_tagid, -1, &stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_bind_text(stmt, 1, tag, -1, SQLITE_STATIC);
    int tag_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) tag_id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (tag_id < 0) { pthread_mutex_unlock(&d->lock); return -1; }

    const char *link = "INSERT OR IGNORE INTO log_tags(log_id, tag_id) VALUES(?, ?);";
    if (sqlite3_prepare_v2(d->db, link, -1, &stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_bind_int(stmt, 1, log_id);
    sqlite3_bind_int(stmt, 2, tag_id);
    if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&d->lock);
    return 0;
}

int db_remove_tag(DB *d, int log_id, const char *tag) {
    if (!d || !d->db) return -1;
    pthread_mutex_lock(&d->lock);
    sqlite3_stmt *stmt = NULL;
    const char *get_tagid = "SELECT id FROM tags WHERE name=? LIMIT 1;";
    if (sqlite3_prepare_v2(d->db, get_tagid, -1, &stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_bind_text(stmt, 1, tag, -1, SQLITE_STATIC);
    int tag_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) tag_id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (tag_id < 0) { pthread_mutex_unlock(&d->lock); return -1; }

    const char *del = "DELETE FROM log_tags WHERE log_id=? AND tag_id=?;";
    if (sqlite3_prepare_v2(d->db, del, -1, &stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_bind_int(stmt, 1, log_id);
    sqlite3_bind_int(stmt, 2, tag_id);
    if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&d->lock);
    return 0;
}

char **db_list_tags(DB *d, int log_id) {
    if (!d || !d->db) return NULL;
    pthread_mutex_lock(&d->lock);
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT tags.name FROM tags JOIN log_tags ON tags.id = log_tags.tag_id WHERE log_tags.log_id = ?;";
    if (sqlite3_prepare_v2(d->db, sql, -1, &stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return NULL; }
    sqlite3_bind_int(stmt, 1, log_id);
    char **arr = NULL;
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = (const char*)sqlite3_column_text(stmt, 0);
        arr = realloc(arr, sizeof(char*) * (n + 2));
        arr[n] = strdup(name);
        n++;
        arr[n] = NULL;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&d->lock);
    return arr;
}

void db_free_string_array(char **arr) {
    if (!arr) return;
    for (size_t i = 0; arr[i]; ++i) free(arr[i]);
    free(arr);
}

int db_insert_log(DB *d, const char *source, const char *unit, const char *message, const char *ts) {
    if (!d || !d->db) return -1;
    pthread_mutex_lock(&d->lock);
    const char *sql = "INSERT INTO logs(source, unit, ts, message) VALUES(?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(d->db, sql, -1, &stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_bind_text(stmt, 1, source, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, unit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, ts, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, message, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&d->lock);
        return -1;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&d->lock);
    return 0;
}

int db_search(DB *d, const char *query, int limit, int offset, sqlite3_stmt **out_stmt) {
    if (!d || !d->db) return -1;
    /* Protect the prepare phase with the DB lock. The caller will step
     * through and finalize the returned statement; we do not hold the
     * lock across that use. */
    pthread_mutex_lock(&d->lock);
    if (!query || query[0] == '\0') {
        const char *sql_recent = "SELECT id, source, unit, ts, message FROM logs ORDER BY ts DESC LIMIT ? OFFSET ?;";
        if (sqlite3_prepare_v2(d->db, sql_recent, -1, out_stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
        sqlite3_bind_int(*out_stmt, 1, limit);
        sqlite3_bind_int(*out_stmt, 2, offset);
        pthread_mutex_unlock(&d->lock);
        return 0;
    }
    const char *sql = "SELECT logs.id, logs.source, logs.unit, logs.ts, logs.message FROM logs JOIN logs_fts ON logs.rowid = logs_fts.rowid WHERE logs_fts MATCH ? ORDER BY ts DESC LIMIT ? OFFSET ?;";
    if (sqlite3_prepare_v2(d->db, sql, -1, out_stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_bind_text(*out_stmt, 1, query, -1, SQLITE_STATIC);
    sqlite3_bind_int(*out_stmt, 2, limit);
    sqlite3_bind_int(*out_stmt, 3, offset);
    pthread_mutex_unlock(&d->lock);
    return 0;
}

int db_get_message(DB *d, int log_id, char **out_message) {
    if (!d || !d->db) return -1;
    pthread_mutex_lock(&d->lock);
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT message FROM logs WHERE id = ? LIMIT 1;";
    if (sqlite3_prepare_v2(d->db, sql, -1, &stmt, NULL) != SQLITE_OK) { pthread_mutex_unlock(&d->lock); return -1; }
    sqlite3_bind_int(stmt, 1, log_id);
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *msg = (const char*)sqlite3_column_text(stmt, 0);
        if (msg) *out_message = strdup(msg); else *out_message = strdup("");
        rc = 0;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&d->lock);
    return rc;
}
