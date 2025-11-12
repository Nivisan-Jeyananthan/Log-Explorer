#include "indexer.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>

// helper: check whether an executable exists in PATH
static int program_in_path(const char *prog) {
    if (!prog) return 0;
    const char *path = getenv("PATH");
    if (!path) return 0;
    char *p = strdup(path);
    if (!p) return 0;
    char *save = p;
    char buf[1024];
    char *tok = strtok(p, ":");
    while (tok) {
        snprintf(buf, sizeof(buf), "%s/%s", tok, prog);
        if (access(buf, X_OK) == 0) {
            free(save);
            return 1;
        }
        tok = strtok(NULL, ":");
    }
    free(save);
    return 0;
}

// Very small JSON helpers (prototype): extract value for a top-level key like "MESSAGE" or "_SYSTEMD_UNIT"
static char *json_extract_value(const char *json, const char *key) {
    // Find "key"
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return NULL;
    p = strchr(p + strlen(needle), ':');
    if (!p) return NULL;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    // value may be string or number; handle string
    if (*p == '"') {
        p++;
        const char *start = p;
        while (*p && *p != '"') p++;
        size_t len = p - start;
        char *out = malloc(len + 1);
        if (!out) return NULL;
        memcpy(out, start, len);
        out[len] = '\0';
        return out;
    }
    return NULL;
}

static void ingest_log(DB *db, const char *source, const char *unit, const char *message, const char *ts) {
    if (!message) return;
    db_insert_log(db, source ? source : "unknown", unit ? unit : "", message, ts ? ts : "");
}

// Journal reader: uses `journalctl -o json -f` to follow new entries.
static char *read_journal_cursor(void) {
    FILE *f = fopen(".journal_cursor", "r");
    if (!f) return NULL;
    char buf[4096];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return NULL;
    }
    fclose(f);
    // strip newline
    size_t l = strlen(buf);
    if (l && buf[l-1] == '\n') buf[l-1] = '\0';
    return strdup(buf);
}

static void write_journal_cursor(const char *cursor) {
    if (!cursor) return;
    FILE *f = fopen(".journal_cursor", "w");
    if (!f) return;
    fprintf(f, "%s\n", cursor);
    fclose(f);
}

static pthread_t g_jth = 0;
static pthread_t g_fth = 0;
static volatile int g_indexer_running = 0;

static void *journal_thread(void *arg) {
    DB *db = (DB*)arg;
    // Try to follow journal; if not available or permissions, exit thread.
    char *cursor = read_journal_cursor();
    char cmd[8192];
    if (cursor) {
        snprintf(cmd, sizeof(cmd), "journalctl -o json --after-cursor='%s' -f", cursor);
    } else {
        snprintf(cmd, sizeof(cmd), "journalctl -o json -f");
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    g_indexer_running = 1;
    while (g_indexer_running && (len = getline(&line, &cap, fp)) > 0) {
        // line is a JSON object
        char *msg = json_extract_value(line, "MESSAGE");
        char *unit = json_extract_value(line, "_SYSTEMD_UNIT");
        char *ts = json_extract_value(line, "__REALTIME_TIMESTAMP");
        char *cursor_line = json_extract_value(line, "__CURSOR");
        // convert microsecond epoch to iso string? keep raw for prototype
        ingest_log(db, "journal", unit, msg, ts);
        if (cursor_line) {
            write_journal_cursor(cursor_line);
        }
        free(msg); free(unit); free(ts);
        free(cursor_line);
    }
    free(line);
    pclose(fp);
    free(cursor);
    return NULL;
}

// Simple file tailer: read plain text files under /var/log and ingest lines. Tracks inode+offset in a simple .offset file per logfile.
// helper: ensure .offsets directory exists
static void ensure_offsets_dir(void) {
    struct stat st;
    if (stat(".offsets", &st) != 0) {
        mkdir(".offsets", 0700);
    }
}

// read stored offset for a file (returns -1 if none)
static off_t read_offset(const char *basename) {
    char path[1024];
    snprintf(path, sizeof(path), ".offsets/%s.offset", basename);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    long off = -1;
    if (fscanf(f, "%ld", &off) != 1) off = -1;
    fclose(f);
    return (off >= 0) ? (off_t)off : -1;
}

static void write_offset(const char *basename, off_t off) {
    char path[1024];
    snprintf(path, sizeof(path), ".offsets/%s.offset", basename);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "%ld\n", (long)off);
    fclose(f);
}

static void tail_file_once(DB *db, const char *path) {
    ensure_offsets_dir();
    const char *fname = strrchr(path, '/');
    const char *basename = fname ? fname + 1 : path;
    off_t stored = read_offset(basename);

    int fd = open(path, O_RDONLY);
    if (fd < 0) return;
    struct stat st;
    if (fstat(fd, &st) != 0) { close(fd); return; }
    off_t size = st.st_size;
    off_t start = 0;
    if (stored >= 0 && stored <= size) start = stored;

    FILE *f = fdopen(fd, "r");
    if (!f) { close(fd); return; }
    if (start > 0) fseeko(f, start, SEEK_SET);

    char *line = NULL;
    size_t cap = 0;
    off_t lastpos = start;
    while (getline(&line, &cap, f) > 0) {
        const char *unit = basename;
        ingest_log(db, path, unit, line, "");
        lastpos = ftello(f);
    }
    if (line) free(line);
    write_offset(basename, lastpos);
    fclose(f);
}

static void *varlog_thread(void *arg) {
    DB *db = (DB*)arg;
    const char *dir = "/var/log";
    // For prototype: iterate files directly in /var/log (no recursion)
    DIR *d = opendir(dir);
    if (!d) return NULL;
    struct dirent *ent;
    char path[1024];
    while (g_indexer_running && (ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        tail_file_once(db, path);
    }
    closedir(d);
    return NULL;
}

int indexer_start(DB *db) {
    if (g_indexer_running) return 0;
    g_indexer_running = 1;
    /* Start journal thread only if journalctl is available in PATH. In
     * sandboxed environments (Flatpak build/run sandbox) journalctl may not
     * be present; avoid spawning it and instead continue with file-based
     * indexing. This makes the app behave more gracefully when run in a
     * restricted environment. */
    if (program_in_path("journalctl")) {
        if (pthread_create(&g_jth, NULL, journal_thread, db) != 0) {
            g_jth = 0;
        }
    } else {
        fprintf(stderr, "indexer: journalctl not found in PATH, skipping journal thread\n");
        g_jth = 0;
    }
    if (pthread_create(&g_fth, NULL, varlog_thread, db) != 0) {
        g_fth = 0;
    }
    return 0;
}

int indexer_stop(void) {
    if (!g_indexer_running) return 0;
    g_indexer_running = 0;
    // join threads if they were started
    if (g_jth) pthread_join(g_jth, NULL);
    if (g_fth) pthread_join(g_fth, NULL);
    g_jth = g_fth = 0;
    return 0;
}
