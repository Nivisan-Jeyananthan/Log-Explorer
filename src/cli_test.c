#include <stdio.h>
#include "db.h"

int main(void) {
    DB db;
    if (db_open(&db, "./test.db") != 0) {
        fprintf(stderr, "db_open failed\n");
        return 1;
    }
    db_insert_log(&db, "cli", "test.service", "CLI test message: hello world", "2025-10-22T12:00:00Z");

    sqlite3_stmt *stmt = NULL;
    if (db_search(&db, "hello", 100, 0, &stmt) == 0) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char *source = (const char*)sqlite3_column_text(stmt, 1);
            const char *unit = (const char*)sqlite3_column_text(stmt, 2);
            const char *ts = (const char*)sqlite3_column_text(stmt, 3);
            const char *message = (const char*)sqlite3_column_text(stmt, 4);
            printf("%d %s %s %s %s\n", id, source, unit, ts, message);
        }
        sqlite3_finalize(stmt);
    }

    db_close(&db);
    return 0;
}
