#include "ui.h"
#include <stdio.h>

static const int PAGE_SIZE = 100;

// helper to set/get offset stored on window
static int win_get_offset(GtkWidget *win) {
    gpointer p = g_object_get_data(G_OBJECT(win), "results_offset");
    if (!p) return 0;
    return GPOINTER_TO_INT(p);
}
static void win_set_offset(GtkWidget *win, int off) {
    g_object_set_data(G_OBJECT(win), "results_offset", GINT_TO_POINTER(off));
}

static void on_search_activate(GtkWidget *entry, gpointer user_data) {
    DB *db = (DB*)user_data;
    const char *q = gtk_editable_get_text(GTK_EDITABLE(entry));
    GtkWidget *win = g_object_get_data(G_OBJECT(entry), "main_window");
    if (!win) win = g_object_get_data(G_OBJECT(entry), "results_main");
    if (!win) win = g_object_get_data(G_OBJECT(entry), "main_window");
    // reset offset on new search
    win_set_offset(win, 0);
    int offset = win_get_offset(win);

    sqlite3_stmt *stmt = NULL;
    if (db_search(db, q, PAGE_SIZE, offset, &stmt) != 0) {
        g_warning("Search failed");
        return;
    }

    GtkWidget *list = g_object_get_data(G_OBJECT(entry), "results_list");
    if (!list) {
        g_warning("results_list not found on entry");
        sqlite3_finalize(stmt);
        return;
    }
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
    // if offset==0 clear existing rows, otherwise append
    if (offset == 0) gtk_list_store_clear(store);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        gint id = sqlite3_column_int(stmt, 0);
        const char *source = (const char*)sqlite3_column_text(stmt, 1);
        const char *unit = (const char*)sqlite3_column_text(stmt, 2);
        const char *ts = (const char*)sqlite3_column_text(stmt, 3);
        const char *message = (const char*)sqlite3_column_text(stmt, 4);
        // truncate preview to keep UI snappy
        char preview[512];
        if (!message) message = "";
        if ((int)strlen(message) > 200) {
            strncpy(preview, message, 197);
            preview[197] = '\0';
            strcat(preview, "...");
        } else {
            strncpy(preview, message, sizeof(preview)-1);
            preview[sizeof(preview)-1] = '\0';
        }
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, id,
            1, source ? source : "",
            2, unit ? unit : "",
            3, ts ? ts : "",
            4, preview,
            -1);
    }
    sqlite3_finalize(stmt);
    // if we filled PAGE_SIZE rows, enable Load More button
    GtkWidget *load_more = g_object_get_data(G_OBJECT(win), "load_more_btn");
    if (load_more) {
        int rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
        if (rows >= PAGE_SIZE + offset) {
            gtk_widget_set_sensitive(load_more, TRUE);
        } else {
            gtk_widget_set_sensitive(load_more, FALSE);
        }
    }
}

// Load next page and append to results
static void on_load_more_clicked(GtkWidget *button, gpointer user_data) {
    GtkWidget *search = (GtkWidget*)user_data;
    GtkWidget *win = g_object_get_data(G_OBJECT(search), "main_window");
    if (!win) return;
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    const char *q = gtk_editable_get_text(GTK_EDITABLE(search));
    int offset = win_get_offset(win);
    offset += PAGE_SIZE;
    sqlite3_stmt *stmt = NULL;
    if (db_search(db, q, PAGE_SIZE, offset, &stmt) != 0) {
        g_warning("Load more failed");
        return;
    }

    GtkWidget *list = g_object_get_data(G_OBJECT(search), "results_list");
    if (!list) {
        g_warning("results_list not found on entry");
        sqlite3_finalize(stmt);
        return;
    }
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        gint id = sqlite3_column_int(stmt, 0);
        const char *source = (const char*)sqlite3_column_text(stmt, 1);
        const char *unit = (const char*)sqlite3_column_text(stmt, 2);
        const char *ts = (const char*)sqlite3_column_text(stmt, 3);
        const char *message = (const char*)sqlite3_column_text(stmt, 4);
        char preview[512];
        if (!message) message = "";
        if ((int)strlen(message) > 200) {
            strncpy(preview, message, 197);
            preview[197] = '\0';
            strcat(preview, "...");
        } else {
            strncpy(preview, message, sizeof(preview)-1);
            preview[sizeof(preview)-1] = '\0';
        }
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, id,
            1, source ? source : "",
            2, unit ? unit : "",
            3, ts ? ts : "",
            4, preview,
            -1);
    }
    sqlite3_finalize(stmt);
    win_set_offset(win, offset);
    // adjust load more sensitivity
    GtkWidget *load_more = g_object_get_data(G_OBJECT(win), "load_more_btn");
    if (load_more) {
        int rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
        if (rows >= PAGE_SIZE + offset) gtk_widget_set_sensitive(load_more, TRUE);
        else gtk_widget_set_sensitive(load_more, FALSE);
    }
}

// Helper: set message text in details text view
static void set_message_view(GtkWidget *win, const char *message) {
    GtkWidget *tv = g_object_get_data(G_OBJECT(win), "message_view");
    if (!tv) return;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_set_text(buf, message ? message : "", -1);
}

// Helper: populate tag_store for a given log id
static void populate_tags(GtkWidget *win, DB *db, int log_id) {
    GtkListStore *tag_store = g_object_get_data(G_OBJECT(win), "tag_store");
    if (!tag_store) return;
    gtk_list_store_clear(tag_store);
    char **tags = db_list_tags(db, log_id);
    if (!tags) return;
    for (size_t i = 0; tags[i]; ++i) {
        GtkTreeIter iter;
        gtk_list_store_append(tag_store, &iter);
        gtk_list_store_set(tag_store, &iter, 0, tags[i], -1);
    }
    db_free_string_array(tags);
}

void on_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) return;
    int id;
    gchar *source;
    gchar *unit;
    gchar *ts;
    gchar *message;
    gtk_tree_model_get(model, &iter, 0, &id, 1, &source, 2, &unit, 3, &ts, 4, &message, -1);

    GtkWidget *win = (GtkWidget*)user_data;
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    // fetch full message from DB to avoid relying on truncated preview
    char *full = NULL;
    if (db_get_message(db, id, &full) == 0 && full) {
        set_message_view(win, full);
        free(full);
    } else {
        set_message_view(win, message);
    }
    populate_tags(win, db, id);

    g_free(source); g_free(unit); g_free(ts); g_free(message);
}

void on_add_tag_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    GtkWidget *win = (GtkWidget*)user_data;
    GtkWidget *entry = g_object_get_data(G_OBJECT(win), "tag_entry");
    const char *tag = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (!tag || !*tag) return;
    GtkWidget *view = g_object_get_data(G_OBJECT(win), "results_list");
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return;
    int id;
    gtk_tree_model_get(model, &iter, 0, &id, -1);
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    db_add_tag(db, id, tag);
    populate_tags(win, db, id);
    // clear entry after adding
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

void on_remove_tag_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    GtkWidget *win = (GtkWidget*)user_data;
    // remove the selected tag from the tag_view
    GtkWidget *tag_view = g_object_get_data(G_OBJECT(win), "tag_view");
    GtkTreeSelection *tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tag_view));
    GtkTreeModel *tmodel;
    GtkTreeIter titer;
    if (!gtk_tree_selection_get_selected(tsel, &tmodel, &titer)) return;
    char *tag = NULL;
    gtk_tree_model_get(tmodel, &titer, 0, &tag, -1);
    if (!tag) return;
    GtkWidget *view = g_object_get_data(G_OBJECT(win), "results_list");
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
        g_free(tag);
        return;
    }
    int id;
    gtk_tree_model_get(model, &iter, 0, &id, -1);
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    db_remove_tag(db, id, tag);
    g_free(tag);
    populate_tags(win, db, id);
}

GtkWidget *create_main_window(DB *db) {
    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Log Explorer");
    gtk_window_set_default_size(GTK_WINDOW(win), 900, 600);

    GtkWidget *grid = gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(win), grid);

    // Left column: search + results
    GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_grid_attach(GTK_GRID(grid), left_vbox, 0, 0, 1, 2);

    GtkWidget *search = gtk_search_entry_new();
    gtk_box_append(GTK_BOX(left_vbox), search);

    GtkListStore *store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    GtkCellRenderer *r0 = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *c0 = gtk_tree_view_column_new_with_attributes("ID", r0, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), c0);
    GtkTreeViewColumn *c1 = gtk_tree_view_column_new_with_attributes("Source", r0, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), c1);
    GtkTreeViewColumn *c2 = gtk_tree_view_column_new_with_attributes("Unit", r0, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), c2);
    GtkTreeViewColumn *c3 = gtk_tree_view_column_new_with_attributes("TS", r0, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), c3);
    GtkTreeViewColumn *c4 = gtk_tree_view_column_new_with_attributes("Message", r0, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), c4);

    gtk_box_append(GTK_BOX(left_vbox), view);

    // Load More button for pagination
    GtkWidget *load_more = gtk_button_new_with_label("Load more");
    gtk_widget_set_sensitive(load_more, FALSE);
    gtk_box_append(GTK_BOX(left_vbox), load_more);

     /* store results view on the search entry so the callback can access it without
         needing to query the toplevel (avoids potential ABI/header mismatches) */
    g_object_set_data(G_OBJECT(search), "results_list", view);
    g_object_set_data(G_OBJECT(win), "results_list", view);
    // link search entry back to main window
    g_object_set_data(G_OBJECT(search), "main_window", win);
    g_object_set_data(G_OBJECT(win), "load_more_btn", load_more);

    // initialize offset
    win_set_offset(win, 0);

    // Right column: details pane
    GtkWidget *detail_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_grid_attach(GTK_GRID(grid), detail_box, 1, 0, 1, 2);

    GtkWidget *message_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(message_view), FALSE);
    gtk_widget_set_vexpand(message_view, TRUE);
    gtk_box_append(GTK_BOX(detail_box), message_view);

    // Tag controls
    GtkWidget *tag_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(detail_box), tag_hbox);
    GtkWidget *tag_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(tag_hbox), tag_entry);
    GtkWidget *add_btn = gtk_button_new_with_label("Add Tag");
    gtk_box_append(GTK_BOX(tag_hbox), add_btn);
    GtkWidget *remove_btn = gtk_button_new_with_label("Remove Tag");
    gtk_box_append(GTK_BOX(tag_hbox), remove_btn);

    GtkListStore *tag_store = gtk_list_store_new(1, G_TYPE_STRING);
    GtkWidget *tag_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tag_store));
    GtkCellRenderer *tr = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tc = gtk_tree_view_column_new_with_attributes("Tags", tr, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tag_view), tc);
    gtk_widget_set_vexpand(tag_view, FALSE);
    gtk_box_append(GTK_BOX(detail_box), tag_view);

    // Store references for callbacks
    g_object_set_data(G_OBJECT(win), "message_view", message_view);
    g_object_set_data(G_OBJECT(win), "tag_store", tag_store);
    g_object_set_data(G_OBJECT(win), "tag_entry", tag_entry);
    g_object_set_data(G_OBJECT(win), "add_btn", add_btn);
    g_object_set_data(G_OBJECT(win), "remove_btn", remove_btn);
    g_object_set_data(G_OBJECT(win), "tag_view", tag_view);
    g_object_set_data(G_OBJECT(win), "db", db);

    g_signal_connect(search, "activate", G_CALLBACK(on_search_activate), db);

    // Load more callback: load next page
    g_signal_connect(load_more, "clicked", G_CALLBACK(on_load_more_clicked), search);

    /* populate initial results with recent logs */
    on_search_activate(search, db);

    // Selection changed callback
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    g_signal_connect(sel, "changed", G_CALLBACK(on_selection_changed), win);

    // Tag button callbacks
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_tag_clicked), win);
    g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_tag_clicked), win);

    return win;
}
