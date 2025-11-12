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
    (void)factory;
    /* user_data is the main window so we can register label widgets for
     * responsive visibility toggling. */
    GtkWidget *win = (GtkWidget*)user_data;
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
    /* Register these labels on the window so we can toggle visibility when
     * the window becomes narrow. We store GList* under keys on the window. */
    if (win) {
        GList *slist = (GList*)g_object_get_data(G_OBJECT(win), "col_source_labels");
        slist = g_list_prepend(slist, source_label);
        g_object_set_data(G_OBJECT(win), "col_source_labels", slist);
        GList *tlist = (GList*)g_object_get_data(G_OBJECT(win), "col_ts_labels");
        tlist = g_list_prepend(tlist, ts_label);
        g_object_set_data(G_OBJECT(win), "col_ts_labels", tlist);
        /* set initial visibility according to current narrow mode */
        gboolean narrow = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(win), "narrow_mode"));
        gtk_widget_set_visible(source_label, !narrow);
        gtk_widget_set_visible(ts_label, !narrow);
    }
    gtk_list_item_set_child(list_item, h);
}

static void result_factory_bind(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    (void)factory;
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
    /* Ensure newly bound items follow the current responsive visibility state */
    if (user_data) {
        GtkWidget *win = (GtkWidget*)user_data;
        gboolean narrow = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(win), "narrow_mode"));
        gtk_widget_set_visible(source_label, !narrow);
        gtk_widget_set_visible(ts_label, !narrow);
    }
}

static void window_size_allocate_cb(GtkWidget *win, GtkAllocation *alloc, gpointer user_data) {
    (void)user_data;
    int width = alloc->width;
    gboolean narrow = width < 700;
    gboolean prev = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(win), "narrow_mode"));
    if (narrow == prev) return;
    g_object_set_data(G_OBJECT(win), "narrow_mode", GINT_TO_POINTER(narrow));
    /* Toggle visibility for registered labels */
    GList *slist = (GList*)g_object_get_data(G_OBJECT(win), "col_source_labels");
    for (GList *l = slist; l; l = l->next) {
        gtk_widget_set_visible(GTK_WIDGET(l->data), !narrow);
    }
    GList *tlist = (GList*)g_object_get_data(G_OBJECT(win), "col_ts_labels");
    for (GList *l = tlist; l; l = l->next) {
        gtk_widget_set_visible(GTK_WIDGET(l->data), !narrow);
    }
}

/* forward declaration: create_details_window is defined later but used by the
 * results view pressed handler (double-click). Declare it here to avoid an
 * implicit declaration warning/error. */
static void create_details_window(DB *db, LogItem *li);


static void results_view_pressed_cb(GtkGesture *gesture, int n_press, double x, double y, gpointer user_data) {
    (void)gesture; (void)x; (void)y;
    g_debug("results_view_pressed: n_press=%d", n_press);
    if (n_press != 2) return;
    GtkWidget *view = GTK_WIDGET(user_data);
    if (!view) return;
    GtkSingleSelection *sel = GTK_SINGLE_SELECTION(g_object_get_data(G_OBJECT(view), "results_sel"));
    if (!sel) {
        g_debug("results_view_pressed: no single selection stored on view");
        return;
    }
    GObject *item = gtk_single_selection_get_selected_item(sel);
    if (!item) {
        g_debug("results_view_pressed: no item selected on double-click");
        return;
    }
    LogItem *li = LOG_ITEM(item);
    GtkWidget *win = g_object_get_data(G_OBJECT(view), "main_window");
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    g_debug("results_view_pressed: opening details for id=%d", li->id);
    create_details_window(db, li);
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
        /* Use GListModel API to get the number of items in the GListStore.
         * Previously the code used a GtkTreeModel API which returns 0 for
         * GListStore and caused the Load More button to remain disabled. */
        int rows = g_list_model_get_n_items((GListModel*)store);
        if (rows >= PAGE_SIZE + offset) gtk_widget_set_sensitive(load_more, TRUE);
        else gtk_widget_set_sensitive(load_more, FALSE);
    }
    g_debug("search: after populate rows=%d offset=%d", g_list_model_get_n_items((GListModel*)store), offset);
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
    g_debug("load_more: appended page offset=%d rows=%d", offset, g_list_model_get_n_items((GListModel*)store));
}

// Helper: set message text in details text view
static void set_message_view(GtkWidget *win, const char *message) {
    /* If the main window contains a full message view (legacy layout),
     * write into it. Otherwise fall back to a compact preview label placed
     * under the results list. This supports the new separate details
     * window while preserving older behaviour if present. */
    GtkWidget *tv = g_object_get_data(G_OBJECT(win), "message_view");
    if (tv) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
        gtk_text_buffer_set_text(buf, message ? message : "", -1);
        return;
    }
    GtkWidget *preview = g_object_get_data(G_OBJECT(win), "preview_label");
    if (!preview) return;
    /* gtk_label_set_text is safe here; preview label is a simple compact
     * representation of the message chosen to fit inline with results. */
    gtk_label_set_text(GTK_LABEL(preview), message ? message : "");
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
    /* Selection no longer updates an inline details pane; details and
     * tagging are shown in a separate window opened on double-click. */
    (void)li; (void)win;
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

/* New detail-window tagging helpers: operate on a details window instance which
 * stores 'db' and 'log_id' in its object data. */
static void detail_populate_tags(GtkWidget *detail_win) {
    DB *db = g_object_get_data(G_OBJECT(detail_win), "db");
    int log_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(detail_win), "log_id"));
    GtkWidget *tag_view = g_object_get_data(G_OBJECT(detail_win), "tag_view");
    if (!db || !tag_view) return;
    char **tags = db_list_tags(db, log_id);
    if (!tags) return;
    GListStore *tag_store = g_list_store_new(tag_item_get_type());
    for (size_t i = 0; tags[i]; ++i) {
        TagItem *ti = tag_item_new(tags[i]);
        g_list_store_append(tag_store, G_OBJECT(ti));
        g_object_unref(ti);
    }
    db_free_string_array(tags);
    GtkSingleSelection *tsel = GTK_SINGLE_SELECTION(gtk_single_selection_new((GListModel*)tag_store));
    gtk_list_view_set_model(GTK_LIST_VIEW(tag_view), (GtkSelectionModel*)tsel);
    g_object_unref(tag_store);
}

static void on_add_tag_detail_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    GtkWidget *win = (GtkWidget*)user_data;
    GtkWidget *entry = g_object_get_data(G_OBJECT(win), "tag_entry");
    const char *tag = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (!tag || !*tag) return;
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    int log_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(win), "log_id"));
    if (!db) return;
    db_add_tag(db, log_id, tag);
    detail_populate_tags(win);
    gtk_editable_set_text(GTK_EDITABLE(entry), "");
}

static void on_remove_tag_detail_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    GtkWidget *win = (GtkWidget*)user_data;
    GtkWidget *tag_view = g_object_get_data(G_OBJECT(win), "tag_view");
    if (!tag_view) return;
    GtkSelectionModel *sel_model = gtk_list_view_get_model(GTK_LIST_VIEW(tag_view));
    if (!sel_model || !GTK_IS_SINGLE_SELECTION(sel_model)) return;
    GtkSingleSelection *single_sel = GTK_SINGLE_SELECTION(sel_model);
    GObject *titem = gtk_single_selection_get_selected_item(single_sel);
    if (!titem) return;
    TagItem *ti = TAG_ITEM(titem);
    const char *tag = ti->name;
    if (!tag) return;
    DB *db = g_object_get_data(G_OBJECT(win), "db");
    int log_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(win), "log_id"));
    if (!db) return;
    db_remove_tag(db, log_id, tag);
    detail_populate_tags(win);
}

/* Create a new top-level window that shows full message and tag controls */
static void create_details_window(DB *db, LogItem *li) {
    if (!db || !li) return;
    GtkWidget *dwin = gtk_window_new();
    char title[128];
    snprintf(title, sizeof(title), "Log #%d Details", li->id);
    gtk_window_set_title(GTK_WINDOW(dwin), title);
    gtk_window_set_default_size(GTK_WINDOW(dwin), 600, 400);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_window_set_child(GTK_WINDOW(dwin), vbox);

    GtkWidget *msg_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(msg_view), FALSE);
    GtkWidget *msg_scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(msg_scroller), msg_view);
    gtk_widget_set_hexpand(msg_scroller, TRUE);
    gtk_widget_set_vexpand(msg_scroller, TRUE);
    gtk_box_append(GTK_BOX(vbox), msg_scroller);

    /* Tag controls */
    GtkWidget *tag_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(vbox), tag_hbox);
    GtkWidget *tag_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(tag_hbox), tag_entry);
    GtkWidget *add_btn = gtk_button_new_with_label("Add Tag");
    gtk_box_append(GTK_BOX(tag_hbox), add_btn);
    GtkWidget *remove_btn = gtk_button_new_with_label("Remove Tag");
    gtk_box_append(GTK_BOX(tag_hbox), remove_btn);

    /* Tag list */
    GListStore *tag_store = g_list_store_new(tag_item_get_type());
    GtkListItemFactory *tag_factory = gtk_signal_list_item_factory_new();
    g_signal_connect(tag_factory, "setup", G_CALLBACK(tag_factory_setup), NULL);
    g_signal_connect(tag_factory, "bind", G_CALLBACK(tag_factory_bind), NULL);
    GtkSingleSelection *tag_sel = GTK_SINGLE_SELECTION(gtk_single_selection_new((GListModel*)tag_store));
    GtkWidget *tag_view = gtk_list_view_new((GtkSelectionModel*)tag_sel, GTK_LIST_ITEM_FACTORY(tag_factory));
    gtk_widget_set_size_request(tag_view, 150, 100);
    GtkWidget *tag_scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tag_scroller), tag_view);
    gtk_box_append(GTK_BOX(vbox), tag_scroller);

    /* store references on detail window */
    g_object_set_data(G_OBJECT(dwin), "db", db);
    g_object_set_data(G_OBJECT(dwin), "log_id", GINT_TO_POINTER(li->id));
    g_object_set_data(G_OBJECT(dwin), "tag_entry", tag_entry);
    g_object_set_data(G_OBJECT(dwin), "tag_view", tag_view);

    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_tag_detail_clicked), dwin);
    g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_tag_detail_clicked), dwin);

    /* populate message and tags */
    char *full = NULL;
    if (db_get_message(db, li->id, &full) == 0 && full) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(msg_view));
        gtk_text_buffer_set_text(buf, full, -1);
        free(full);
    } else {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(msg_view));
        gtk_text_buffer_set_text(buf, li->preview ? li->preview : "", -1);
    }

    detail_populate_tags(dwin);
    gtk_widget_show(dwin);
}

GtkWidget *create_main_window(DB *db) {
    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Log Explorer");
    /* Use a slightly smaller default size and allow the window to be resized
     * by the user. Some desktop environments may create a non-resizable
     * window by default depending on how the app is started; make it
     * explicit here. */
    gtk_window_set_default_size(GTK_WINDOW(win), 800, 600);
    gtk_window_set_resizable(GTK_WINDOW(win), TRUE);

    /* Put the main content inside a scrolled window; this keeps the UI
     * simple (single top-level window with the results) while details and
     * tagging live in their own top-level window opened on double-click. */
    GtkWidget *scroller = gtk_scrolled_window_new();
    gtk_window_set_child(GTK_WINDOW(win), scroller);

    // Left column: search + results
    GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_hexpand(left_vbox, TRUE);
    gtk_widget_set_vexpand(left_vbox, TRUE);
    /* put the left column into the scroller */
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), left_vbox);

    GtkWidget *search = gtk_search_entry_new();
    gtk_box_append(GTK_BOX(left_vbox), search);

    /* Results model/view using modern GtkListView + GListStore */
    GListStore *results_store = g_list_store_new(LOG_ITEM_TYPE);
    GtkListItemFactory *res_factory = gtk_signal_list_item_factory_new();
    g_signal_connect(res_factory, "setup", G_CALLBACK(result_factory_setup), win);
    g_signal_connect(res_factory, "bind", G_CALLBACK(result_factory_bind), win);
    /* Wrap the store in a GtkSingleSelection to get selection support */
    GtkSingleSelection *results_sel = GTK_SINGLE_SELECTION(gtk_single_selection_new((GListModel*)results_store));
    GtkWidget *view = gtk_list_view_new((GtkSelectionModel*)results_sel, GTK_LIST_ITEM_FACTORY(res_factory));
    gtk_widget_set_vexpand(view, TRUE);
    /* store references */
    g_object_set_data(G_OBJECT(view), "results_store", results_store);
    g_object_set_data(G_OBJECT(view), "results_sel", results_sel);
    /* also store results view on search entry so callbacks can find it */

    gtk_widget_set_hexpand(view, TRUE);
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
    /* Initialize narrow mode flag and size-allocate handler */
    g_object_set_data(G_OBJECT(win), "narrow_mode", GINT_TO_POINTER(0));
    g_signal_connect(win, "size-allocate", G_CALLBACK(window_size_allocate_cb), NULL);

    /* Add a double-click gesture to open details in a separate window. */
    GtkGesture *g = gtk_gesture_click_new();
    g_signal_connect(g, "pressed", G_CALLBACK(results_view_pressed_cb), view);
    gtk_widget_add_controller(view, GTK_EVENT_CONTROLLER(g));

    // initialize offset
    win_set_offset(win, 0);
        /* keep DB pointer on the main window for callbacks */
        g_object_set_data(G_OBJECT(win), "db", db);

    g_signal_connect(search, "activate", G_CALLBACK(on_search_activate), db);

    // Load more callback: load next page
    g_signal_connect(load_more, "clicked", G_CALLBACK(on_load_more_clicked), search);

    /* populate initial results with recent logs */
    on_search_activate(search, db);

    // Selection changed callback is handled via GtkSingleSelection notify on the model

    // No tag controls in the main window; tagging is handled in the details window

    return win;
}
