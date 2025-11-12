#include "ui.h"
#include <stdio.h>
#include <glib-object.h>

static const int PAGE_SIZE = 100;

/* Small GObject to represent a log item in the list model. */
typedef struct _LogItem {
    GObject parent_instance;
    int id;
    gchar *source;
    gchar *unit;
    gchar *ts;
    gchar *preview;
} LogItem;

typedef struct _LogItemClass { GObjectClass parent_class; } LogItemClass;

G_DEFINE_TYPE(LogItem, log_item, G_TYPE_OBJECT)

#define LOG_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), log_item_get_type(), LogItem))
#define LOG_ITEM_TYPE (log_item_get_type())

static void log_item_dispose(GObject *object) {
    LogItem *self = (LogItem*)object;
    g_free(self->source);
    g_free(self->unit);
    g_free(self->ts);
    g_free(self->preview);
    G_OBJECT_CLASS(log_item_parent_class)->dispose(object);
}

static void log_item_init(LogItem *self) {
    self->id = -1;
    self->source = NULL;
    self->unit = NULL;
    self->ts = NULL;
    self->preview = NULL;
}

static void log_item_class_init(LogItemClass *klass) {
    GObjectClass *oclass = G_OBJECT_CLASS(klass);
    oclass->dispose = log_item_dispose;
}

static LogItem *log_item_new(int id, const char *source, const char *unit, const char *ts, const char *preview) {
    LogItem *li = g_object_new(log_item_get_type(), NULL);
    li->id = id;
    li->source = source ? g_strdup(source) : g_strdup("");
    li->unit = unit ? g_strdup(unit) : g_strdup("");
    li->ts = ts ? g_strdup(ts) : g_strdup("");
    li->preview = preview ? g_strdup(preview) : g_strdup("");
    return li;
}

/* Tag item as simple GObject wrapping a string */
typedef struct _TagItem { GObject parent_instance; gchar *name; } TagItem;
typedef struct _TagItemClass { GObjectClass parent_class; } TagItemClass;
G_DEFINE_TYPE(TagItem, tag_item, G_TYPE_OBJECT)
#define TAG_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), tag_item_get_type(), TagItem))
#define TAG_ITEM_TYPE (tag_item_get_type())
static void tag_item_dispose(GObject *object) {
    TagItem *t = (TagItem*)object;
    g_free(t->name);
    G_OBJECT_CLASS(tag_item_parent_class)->dispose(object);
}
static void tag_item_init(TagItem *t) { t->name = NULL; }
static void tag_item_class_init(TagItemClass *k) { GObjectClass *oc = G_OBJECT_CLASS(k); oc->dispose = tag_item_dispose; }
static TagItem *tag_item_new(const char *name) { TagItem *t = g_object_new(tag_item_get_type(), NULL); t->name = name ? g_strdup(name) : g_strdup(""); return t; }

// helper to set/get offset stored on window
static int win_get_offset(GtkWidget *win) {
    gpointer p = g_object_get_data(G_OBJECT(win), "results_offset");
    if (!p) return 0;
    return GPOINTER_TO_INT(p);
}
static void win_set_offset(GtkWidget *win, int off) {
    g_object_set_data(G_OBJECT(win), "results_offset", GINT_TO_POINTER(off));
}

/* Result list item factory callbacks */
static void result_factory_setup(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    (void)factory; (void)user_data;
    GtkWidget *h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *id_label = gtk_label_new(NULL);
    GtkWidget *source_label = gtk_label_new(NULL);
    GtkWidget *unit_label = gtk_label_new(NULL);
    GtkWidget *ts_label = gtk_label_new(NULL);
    GtkWidget *preview_label = gtk_label_new(NULL);
    gtk_widget_set_hexpand(preview_label, TRUE);
    gtk_box_append(GTK_BOX(h), id_label);
    gtk_box_append(GTK_BOX(h), source_label);
    gtk_box_append(GTK_BOX(h), unit_label);
    gtk_box_append(GTK_BOX(h), ts_label);
    gtk_box_append(GTK_BOX(h), preview_label);
    g_object_set_data(G_OBJECT(list_item), "id_label", id_label);
    g_object_set_data(G_OBJECT(list_item), "source_label", source_label);
    g_object_set_data(G_OBJECT(list_item), "unit_label", unit_label);
    g_object_set_data(G_OBJECT(list_item), "ts_label", ts_label);
    g_object_set_data(G_OBJECT(list_item), "preview_label", preview_label);
    gtk_list_item_set_child(list_item, h);
}

static void result_factory_bind(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    (void)factory; (void)user_data;
    GObject *item = gtk_list_item_get_item(list_item);
    if (!item) return;
    LogItem *li = LOG_ITEM(item);
    GtkWidget *id_label = g_object_get_data(G_OBJECT(list_item), "id_label");
    GtkWidget *source_label = g_object_get_data(G_OBJECT(list_item), "source_label");
    GtkWidget *unit_label = g_object_get_data(G_OBJECT(list_item), "unit_label");
    GtkWidget *ts_label = g_object_get_data(G_OBJECT(list_item), "ts_label");
    GtkWidget *preview_label = g_object_get_data(G_OBJECT(list_item), "preview_label");
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", li->id);
    gtk_label_set_text(GTK_LABEL(id_label), buf);
    gtk_label_set_text(GTK_LABEL(source_label), li->source ? li->source : "");
    gtk_label_set_text(GTK_LABEL(unit_label), li->unit ? li->unit : "");
    gtk_label_set_text(GTK_LABEL(ts_label), li->ts ? li->ts : "");
    gtk_label_set_text(GTK_LABEL(preview_label), li->preview ? li->preview : "");
}

/* Tag list factory callbacks */
static void tag_factory_setup(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    (void)factory; (void)user_data;
    GtkWidget *lbl = gtk_label_new(NULL);
    gtk_list_item_set_child(list_item, lbl);
    g_object_set_data(G_OBJECT(list_item), "tag_label", lbl);
}
static void tag_factory_bind(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    (void)factory; (void)user_data;
    GObject *item = gtk_list_item_get_item(list_item);
    if (!item) return;
    TagItem *t = TAG_ITEM(item);
    GtkWidget *lbl = g_object_get_data(G_OBJECT(list_item), "tag_label");
    gtk_label_set_text(GTK_LABEL(lbl), t->name ? t->name : "");
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

    GtkWidget *view = g_object_get_data(G_OBJECT(entry), "results_list");
    if (!view) {
        g_warning("results_list not found on entry");
        sqlite3_finalize(stmt);
        return;
    }
    GListStore *store = g_object_get_data(G_OBJECT(view), "results_store");
    if (!store) {
        g_warning("results_store not found on view");
        sqlite3_finalize(stmt);
        return;
    }
    // if offset==0 clear existing rows, otherwise append
    if (offset == 0) {
        while (g_list_model_get_n_items((GListModel*)store) > 0) g_list_store_remove(store, 0);
    }

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
        LogItem *li = log_item_new(id, source ? source : "", unit ? unit : "", ts ? ts : "", preview);
        g_list_store_append(store, G_OBJECT(li));
        g_object_unref(li);
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
    (void)button;
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
    GtkWidget *view = g_object_get_data(G_OBJECT(search), "results_list");
    if (!view) {
        g_warning("results_list not found on entry");
        sqlite3_finalize(stmt);
        return;
    }
    GListStore *store = g_object_get_data(G_OBJECT(view), "results_store");
    if (!store) { sqlite3_finalize(stmt); return; }

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
        LogItem *li = log_item_new(id, source ? source : "", unit ? unit : "", ts ? ts : "", preview);
        g_list_store_append(store, G_OBJECT(li));
        g_object_unref(li);
    }
    sqlite3_finalize(stmt);
    win_set_offset(win, offset);
    // adjust load more sensitivity
    GtkWidget *load_more = g_object_get_data(G_OBJECT(win), "load_more_btn");
    if (load_more) {
        int rows = g_list_model_get_n_items((GListModel*)store);
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
    GtkWidget *tag_view = g_object_get_data(G_OBJECT(win), "tag_view");
    if (!tag_view) return;
    char **tags = db_list_tags(db, log_id);
    if (!tags) return;
    GListStore *tag_store = g_list_store_new(TAG_ITEM_TYPE);
    for (size_t i = 0; tags[i]; ++i) {
        TagItem *ti = tag_item_new(tags[i]);
        g_list_store_append(tag_store, G_OBJECT(ti));
        g_object_unref(ti);
    }
    db_free_string_array(tags);
    /* Wrap the tag store in a GtkSingleSelection so the view supports selection */
    GtkSingleSelection *tsel = GTK_SINGLE_SELECTION(gtk_single_selection_new((GListModel*)tag_store));
    gtk_list_view_set_model(GTK_LIST_VIEW(tag_view), (GtkSelectionModel*)tsel);
    g_object_unref(tag_store);
}

/* Selection notify callback for the main results single selection */
static void selection_notify_cb(GObject *sel, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    GtkSingleSelection *s = GTK_SINGLE_SELECTION(sel);
    GObject *item = gtk_single_selection_get_selected_item(s);
    if (!item) return;
    LogItem *li = LOG_ITEM(item);
    GtkWidget *win = GTK_WIDGET(user_data);
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    char *full = NULL;
    if (db_get_message(db, li->id, &full) == 0 && full) {
        set_message_view(win, full);
        free(full);
    } else {
        set_message_view(win, li->preview);
    }
    populate_tags(win, db, li->id);
}

void on_add_tag_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    GtkWidget *win = (GtkWidget*)user_data;
    GtkWidget *entry = g_object_get_data(G_OBJECT(win), "tag_entry");
    const char *tag = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (!tag || !*tag) return;
    GtkWidget *view = g_object_get_data(G_OBJECT(win), "results_list");
    if (!view) return;
    GtkSingleSelection *sel = GTK_SINGLE_SELECTION(g_object_get_data(G_OBJECT(view), "results_sel"));
    if (!sel) return;
    GObject *item = gtk_single_selection_get_selected_item(sel);
    if (!item) return;
    LogItem *li = LOG_ITEM(item);
    int id = li->id;
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    db_add_tag(db, id, tag);
    populate_tags(win, db, id);
    // clear entry after adding
    /* gtk_entry_set_text may not be available/linked in some GTK4 setups;
       use the editable interface which is already used elsewhere. */
    gtk_editable_set_text(GTK_EDITABLE(entry), "");
}

void on_remove_tag_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    GtkWidget *win = (GtkWidget*)user_data;
    // remove the selected tag from the tag_view
    GtkWidget *tag_view = g_object_get_data(G_OBJECT(win), "tag_view");
    if (!tag_view) return;
    GtkSelectionModel *sel_model = gtk_list_view_get_model(GTK_LIST_VIEW(tag_view));
    if (!sel_model) return;
    /* We expect a GtkSingleSelection; extract its GListModel */
    if (!GTK_IS_SINGLE_SELECTION(sel_model)) return;
    GtkSingleSelection *single_sel = GTK_SINGLE_SELECTION(sel_model);
    GListModel *tag_model = gtk_single_selection_get_model(single_sel);
    if (!tag_model) return;
    GObject *titem = gtk_single_selection_get_selected_item(single_sel);
    if (!titem) return;
    TagItem *ti = TAG_ITEM(titem);
    const char *tag = ti->name;
    if (!tag) return;
    GtkWidget *view = g_object_get_data(G_OBJECT(win), "results_list");
    if (!view) return;
    GtkSingleSelection *sel = GTK_SINGLE_SELECTION(g_object_get_data(G_OBJECT(view), "results_sel"));
    if (!sel) return;
    GObject *item = gtk_single_selection_get_selected_item(sel);
    if (!item) return;
    LogItem *li = LOG_ITEM(item);
    int id = li->id;
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    db_remove_tag(db, id, tag);
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

    /* Results model/view using modern GtkListView + GListStore */
    GListStore *results_store = g_list_store_new(LOG_ITEM_TYPE);
    GtkListItemFactory *res_factory = gtk_signal_list_item_factory_new();
    g_signal_connect(res_factory, "setup", G_CALLBACK(result_factory_setup), NULL);
    g_signal_connect(res_factory, "bind", G_CALLBACK(result_factory_bind), NULL);
    /* Wrap the store in a GtkSingleSelection to get selection support */
    GtkSingleSelection *results_sel = GTK_SINGLE_SELECTION(gtk_single_selection_new((GListModel*)results_store));
    GtkWidget *view = gtk_list_view_new((GtkSelectionModel*)results_sel, GTK_LIST_ITEM_FACTORY(res_factory));
    gtk_widget_set_vexpand(view, TRUE);
    /* store references */
    g_object_set_data(G_OBJECT(view), "results_store", results_store);
    g_object_set_data(G_OBJECT(view), "results_sel", results_sel);
    /* also store results view on search entry so callbacks can find it */

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

    /* Connect selection notify so selecting an item updates details */
    g_signal_connect(results_sel, "notify::selected", G_CALLBACK(selection_notify_cb), win);

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

    /* Tag list: GtkListView with simple factory */
    GListStore *tag_store = g_list_store_new(tag_item_get_type());
    GtkListItemFactory *tag_factory = gtk_signal_list_item_factory_new();
    g_signal_connect(tag_factory, "setup", G_CALLBACK(tag_factory_setup), NULL);
    g_signal_connect(tag_factory, "bind", G_CALLBACK(tag_factory_bind), NULL);
    GtkSingleSelection *tag_sel = GTK_SINGLE_SELECTION(gtk_single_selection_new((GListModel*)tag_store));
    GtkWidget *tag_view = gtk_list_view_new((GtkSelectionModel*)tag_sel, GTK_LIST_ITEM_FACTORY(tag_factory));
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

    // Selection changed callback is handled via GtkSingleSelection notify on the model

    // Tag button callbacks
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_tag_clicked), win);
    g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_tag_clicked), win);

    return win;
}
