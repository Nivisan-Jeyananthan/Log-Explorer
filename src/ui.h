#pragma once

#include <gtk/gtk.h>
#include "db.h"

GtkWidget *create_main_window(DB *db);
void on_add_tag_clicked(GtkWidget *button, gpointer user_data);
void on_remove_tag_clicked(GtkWidget *button, gpointer user_data);

// Note: selection handling is implemented internally in ui.c using GtkListView/GListStore.

