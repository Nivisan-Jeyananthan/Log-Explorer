#include <stdio.h>
#include <time.h>
#include "../src/db.h"

static void make_iso_time(char *buf, size_t n) {
    time_t t = time(NULL);
    struct tm tm;
    gmtime_r(&t, &tm);
    strftime(buf, n, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

int main(void) {
    DB db;
    if (db_open(&db, "./log.db") != 0) {
        fprintf(stderr, "db_open failed\n");
        return 1;
    }
    char ts[64];
    make_iso_time(ts, sizeof(ts));
    db_insert_log(&db, "local", "example.service", "Sample log: application started", ts);
    db_insert_log(&db, "local", "example.service", "Sample log: connection established", ts);
    db_insert_log(&db, "syslog", "kernel", "Sample kernel message: usb device connected", ts);
    db_add_tag(&db, 1, "infrastructure");
    db_add_tag(&db, 2, "service");
    db_add_tag(&db, 3, "kernel");
    db_close(&db);
    printf("Inserted sample logs into ./log.db\n");
    return 0;
}
