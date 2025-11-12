#include <gtk/gtk.h>
#include <stdlib.h>
#include "db.h"
#include "ui.h"
#include "indexer.h"

static void app_activate(GApplication *app, gpointer user_data) {
    DB *db = (DB*)user_data;
    GtkWidget *win = create_main_window(db);
    gtk_window_set_application(GTK_WINDOW(win), GTK_APPLICATION(app));
    gtk_window_present(GTK_WINDOW(win));
}

int main(int argc, char **argv) {
    gtk_init();

    DB db;
    if (db_open(&db, "./log.db") != 0) {
        return 1;
    }

    // Start background indexing (journalctl + /var/log)
    indexer_start(&db);

     /* G_APPLICATION_FLAGS_NONE is deprecated in newer glib; use the replacement
         macro to avoid deprecation warnings. */
     GtkApplication *app = gtk_application_new("org.example.logexplorer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), &db);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    // stop indexer threads before closing DB
    indexer_stop();
    db_close(&db);
    return status;
}
