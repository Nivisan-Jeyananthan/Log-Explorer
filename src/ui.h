#pragma once

#include <gtk/gtk.h>
#include "db.h"

GtkWidget *create_main_window(DB *db);

// UI callbacks used internally (exposed for compilation)
void on_selection_changed(GtkTreeSelection *selection, gpointer user_data);
void on_add_tag_clicked(GtkWidget *button, gpointer user_data);
void on_remove_tag_clicked(GtkWidget *button, gpointer user_data);

