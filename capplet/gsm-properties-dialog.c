/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 1999 Free Software Foundation, Inc.
 * Copyright (C) 2007 Vincent Untz.
 * Copyright (C) 2008 Lucas Rocha.
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
 * Copyright (C) 2012-2021 MATE Developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gsm-properties-dialog.h"

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gsm-app-dialog.h"
#include "gsp-app-manager.h"
#include "gsp-app.h"

#define STARTUP_APP_ICON "system-run"

#define SPC_CONFIG_SCHEMA "org.mate.session"
#define SPC_AUTOSAVE_KEY "auto-save-session"
#define SPC_SHOW_HIDDEN_KEY "show-hidden-apps"

struct _GsmPropertiesDialog {
  GtkDialog parent;
  GtkBuilder *builder;
  GtkListStore *list_store;
  GtkTreeModel *tree_filter;
  GtkWidget *treeview;
  GtkWidget *notebook;
  GtkWidget *delete_button;
  GtkWidget *edit_button;
  GtkWidget *remember_toggle;
  GtkWidget *show_hidden_toggle;
  GspAppManager *manager;
  GSettings *settings;
};

enum {
  STORE_COL_ENABLED = 0,
  STORE_COL_GICON,
  STORE_COL_DESCRIPTION,
  STORE_COL_APP,
  STORE_COL_SEARCH,
  NUMBER_OF_COLUMNS
};

static void gsm_properties_dialog_finalize(GObject *object);

G_DEFINE_TYPE(GsmPropertiesDialog, gsm_properties_dialog, GTK_TYPE_DIALOG)

static gboolean find_by_app(GtkTreeModel *model, GtkTreeIter *iter,
                            GspApp *app) {
  GspApp *iter_app = NULL;

  if (!gtk_tree_model_get_iter_first(model, iter))
    return FALSE;

  do {
    gtk_tree_model_get(model, iter, STORE_COL_APP, &iter_app, -1);
    if (iter_app == app) {
      g_object_unref(iter_app);
      return TRUE;
    }
    g_object_unref(iter_app);
  } while (gtk_tree_model_iter_next(model, iter));

  return FALSE;
}

static void _fill_iter_from_app(GtkListStore *list_store, GtkTreeIter *iter,
                                GspApp *app) {
  gboolean enabled;
  GIcon *icon;
  const char *description;
  const char *app_name;

  enabled = !gsp_app_get_hidden(app);
  icon = gsp_app_get_icon(app);
  description = gsp_app_get_description(app);
  app_name = gsp_app_get_name(app);

  if (G_IS_THEMED_ICON(icon)) {
    GtkIconTheme *theme;
    const char *const *icon_names;

    theme = gtk_icon_theme_get_default();
    icon_names = g_themed_icon_get_names(G_THEMED_ICON(icon));
    if (icon_names[0] == NULL ||
        !gtk_icon_theme_has_icon(theme, icon_names[0])) {
      g_clear_object(&icon);
    }
  } else if (G_IS_FILE_ICON(icon)) {
    GFile *iconfile;

    iconfile = g_file_icon_get_file(G_FILE_ICON(icon));
    if (!g_file_query_exists(iconfile, NULL)) {
      g_clear_object(&icon);
    }
  }

  if (icon == NULL)
    icon = g_themed_icon_new(STARTUP_APP_ICON);

  gtk_list_store_set(list_store, iter, STORE_COL_ENABLED, enabled,
                     STORE_COL_GICON, icon, STORE_COL_DESCRIPTION, description,
                     STORE_COL_APP, app, STORE_COL_SEARCH, app_name, -1);
  g_object_unref(icon);
}

static void _app_changed(GsmPropertiesDialog *dialog, GspApp *app) {
  GtkTreeIter iter;

  if (!find_by_app(GTK_TREE_MODEL(dialog->list_store), &iter, app))
    return;

  _fill_iter_from_app(dialog->list_store, &iter, app);
}

static void append_app(GsmPropertiesDialog *dialog, GspApp *app) {
  GtkTreeIter iter;

  if (find_by_app(GTK_TREE_MODEL(dialog->list_store), &iter, app))
    return;

  gtk_list_store_append(dialog->list_store, &iter);
  _fill_iter_from_app(dialog->list_store, &iter, app);
  g_signal_connect_swapped(app, "changed", G_CALLBACK(_app_changed), dialog);
}

static void _app_added(GsmPropertiesDialog *dialog, GspApp *app,
                       GspAppManager *manager) {
  (void)manager;
  append_app(dialog, app);
}

static void remove_app(GsmPropertiesDialog *dialog, GspApp *app) {
  GtkTreeIter iter;

  if (!find_by_app(GTK_TREE_MODEL(dialog->list_store), &iter, app))
    return;

  g_signal_handlers_disconnect_by_func(app, _app_changed, dialog);
  gtk_list_store_remove(dialog->list_store, &iter);
}

static void _app_removed(GsmPropertiesDialog *dialog, GspApp *app,
                         GspAppManager *manager) {
  (void)manager;
  remove_app(dialog, app);
}

static void populate_model(GsmPropertiesDialog *dialog) {
  GSList *apps;
  GSList *l;

  apps = gsp_app_manager_get_apps(dialog->manager);
  for (l = apps; l != NULL; l = l->next)
    append_app(dialog, GSP_APP(l->data));
  g_slist_free(apps);
}

static void on_selection_changed(GtkTreeSelection *selection,
                                 GsmPropertiesDialog *dialog) {
  gboolean sel = gtk_tree_selection_get_selected(selection, NULL, NULL);

  gtk_widget_set_sensitive(dialog->edit_button, sel);
  gtk_widget_set_sensitive(dialog->delete_button, sel);
}

static void on_startup_enabled_toggled(GtkCellRendererToggle *cell_renderer,
                                       char *path,
                                       GsmPropertiesDialog *dialog) {
  GtkTreeIter iter;
  GspApp *app = NULL;

  if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(dialog->tree_filter),
                                           &iter, path)) {
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(dialog->tree_filter), &iter, STORE_COL_APP,
                     &app, -1);

  if (app) {
    gsp_app_set_hidden(app, gtk_cell_renderer_toggle_get_active(cell_renderer));
    g_object_unref(app);
  }
}

static void on_drag_data_received(GtkWidget *widget,
                                  GdkDragContext *drag_context, gint x, gint y,
                                  GtkSelectionData *data, guint info,
                                  guint time, GsmPropertiesDialog *dialog) {
  gboolean dnd_success = FALSE;

  if (data != NULL) {
    char **filenames;
    int i;

    filenames = gtk_selection_data_get_uris(data);
    for (i = 0; filenames[i] && filenames[i][0]; i++) {
      /* Return success if at least one file succeeded */
      gboolean file_success;
      file_success = gsp_app_copy_desktop_file(filenames[i]);
      dnd_success = dnd_success || file_success;
    }
    g_strfreev(filenames);
  }
  gtk_drag_finish(drag_context, dnd_success, FALSE, time);
  g_signal_stop_emission_by_name(widget, "drag_data_received");
}

static void on_drag_begin(GtkWidget *widget, GdkDragContext *context,
                          GsmPropertiesDialog *dialog) {
  GtkTreePath *path;
  GtkTreeIter iter;
  GspApp *app;

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(widget), &path, NULL);
  gtk_tree_model_get_iter(GTK_TREE_MODEL(dialog->tree_filter), &iter, path);
  gtk_tree_path_free(path);

  gtk_tree_model_get(GTK_TREE_MODEL(dialog->tree_filter), &iter, STORE_COL_APP,
                     &app, -1);

  if (app) {
    g_object_set_data_full(G_OBJECT(context), "gsp-app", g_object_ref(app),
                           g_object_unref);
    g_object_unref(app);
  }
}

static void on_drag_data_get(GtkWidget *widget, GdkDragContext *context,
                             GtkSelectionData *selection_data, guint info,
                             guint time, GsmPropertiesDialog *dialog) {
  GspApp *app;

  app = g_object_get_data(G_OBJECT(context), "gsp-app");
  if (app) {
    const char *uris[2];
    char *uri = g_filename_to_uri(gsp_app_get_path(app), NULL, NULL);

    uris[0] = uri;
    uris[1] = NULL;
    gtk_selection_data_set_uris(selection_data, (char **)uris);

    g_free(uri);
  }
}

static void on_add_app_clicked(GtkWidget *widget, GsmPropertiesDialog *dialog) {
  GtkWidget *add_dialog;
  char *name;
  char *exec;
  char *comment;
  guint delay;

  add_dialog = gsm_app_dialog_new(GTK_WINDOW(dialog), NULL, NULL, NULL, 0);
  if (gsm_app_dialog_run(GSM_APP_DIALOG(add_dialog), &name, &exec, &comment,
                         &delay)) {
    gsp_app_create(name, comment, exec, delay);
    g_free(name);
    g_free(exec);
    g_free(comment);
  }
}

static void on_delete_app_clicked(GtkWidget *widget,
                                  GsmPropertiesDialog *dialog) {
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GspApp *app = NULL;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->treeview));

  if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(dialog->tree_filter), &iter, STORE_COL_APP,
                     &app, -1);

  if (app) {
    gsp_app_delete(app);
    g_object_unref(app);
  }
}

static void on_edit_app_clicked(GtkWidget *widget,
                                GsmPropertiesDialog *dialog) {
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GspApp *app = NULL;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->treeview));

  if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(dialog->tree_filter), &iter, STORE_COL_APP,
                     &app, -1);

  if (app) {
    GtkWidget *edit_dialog;
    char *name;
    char *exec;
    char *comment;
    guint delay;

    edit_dialog = gsm_app_dialog_new(
        GTK_WINDOW(dialog), gsp_app_get_name(app), gsp_app_get_exec(app),
        gsp_app_get_comment(app), gsp_app_get_delay(app));
    if (gsm_app_dialog_run(GSM_APP_DIALOG(edit_dialog), &name, &exec, &comment,
                           &delay)) {
      gsp_app_update(app, name, comment, exec, delay);
      g_free(name);
      g_free(exec);
      g_free(comment);
    }
    g_object_unref(app);
  }
}

static void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                             GtkTreeViewColumn *column,
                             GsmPropertiesDialog *dialog) {
  on_edit_app_clicked(NULL, dialog);
}

static void on_show_hidden_toggled(GtkToggleButton *togglebutton,
                                   GsmPropertiesDialog *dialog) {
  (void)togglebutton;

  if (dialog->tree_filter)
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(dialog->tree_filter));
}

static void on_save_session_clicked(GtkWidget *widget,
                                    GsmPropertiesDialog *dialog) {
  g_debug("Session saving is not implemented yet!");
}

static gboolean on_main_notebook_scroll_event(GtkWidget *widget,
                                              GdkEventScroll *event) {
  GtkNotebook *notebook = GTK_NOTEBOOK(widget);
  GtkWidget *child, *event_widget, *action_widget;

  child = gtk_notebook_get_nth_page(notebook,
                                    gtk_notebook_get_current_page(notebook));
  if (child == NULL) return FALSE;

  event_widget = gtk_get_event_widget((GdkEvent *)event);

  /* Ignore scroll events from the content of the page */
  if (event_widget == NULL || event_widget == child ||
      gtk_widget_is_ancestor(event_widget, child))
    return FALSE;

  /* And also from the action widgets */
  action_widget = gtk_notebook_get_action_widget(notebook, GTK_PACK_START);
  if (event_widget == action_widget ||
      (action_widget != NULL &&
       gtk_widget_is_ancestor(event_widget, action_widget)))
    return FALSE;

  action_widget = gtk_notebook_get_action_widget(notebook, GTK_PACK_END);
  if (event_widget == action_widget ||
      (action_widget != NULL &&
       gtk_widget_is_ancestor(event_widget, action_widget)))
    return FALSE;

  switch (event->direction) {
    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      gtk_notebook_next_page(notebook);
      break;
    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      gtk_notebook_prev_page(notebook);
      break;
    case GDK_SCROLL_SMOOTH:
      switch (gtk_notebook_get_tab_pos(notebook)) {
        case GTK_POS_LEFT:
        case GTK_POS_RIGHT:
          if (event->delta_y > 0)
            gtk_notebook_next_page(notebook);
          else if (event->delta_y < 0)
            gtk_notebook_prev_page(notebook);
          break;
        case GTK_POS_TOP:
        case GTK_POS_BOTTOM:
          if (event->delta_x > 0)
            gtk_notebook_next_page(notebook);
          else if (event->delta_x < 0)
            gtk_notebook_prev_page(notebook);
          break;
      }
      break;
  }

  return TRUE;
}

static gboolean visible_func(GtkTreeModel *model, GtkTreeIter *iter,
                             gpointer data) {
  gboolean visible = FALSE;
  GtkToggleButton *toggle_button = data;
  GspApp *app;
  gboolean show_hidden;

  show_hidden = gtk_toggle_button_get_active(toggle_button);
  gtk_tree_model_get(model, iter, STORE_COL_APP, &app, -1);
  if (app) {
    gboolean nodisplay;

    nodisplay = gsp_app_get_nodisplay(app);
    visible = show_hidden || !nodisplay;
    g_object_unref(app);
  } else {
    visible = show_hidden;
  }
  return visible;
}

static void setup_dialog(GsmPropertiesDialog *dialog) {
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  GtkTargetList *targetlist;

  dialog->settings = g_settings_new(SPC_CONFIG_SCHEMA);

  g_settings_bind(dialog->settings, SPC_SHOW_HIDDEN_KEY,
                  dialog->show_hidden_toggle, "active",
                  G_SETTINGS_BIND_DEFAULT);

  dialog->list_store =
      gtk_list_store_new(NUMBER_OF_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_ICON,
                         G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_STRING);
  dialog->tree_filter =
      gtk_tree_model_filter_new(GTK_TREE_MODEL(dialog->list_store), NULL);
  g_object_unref(dialog->list_store);

  gtk_tree_model_filter_set_visible_func(
      GTK_TREE_MODEL_FILTER(dialog->tree_filter), visible_func,
      dialog->show_hidden_toggle, NULL);

  gtk_tree_view_set_model(GTK_TREE_VIEW(dialog->treeview), dialog->tree_filter);
  g_object_unref(dialog->tree_filter);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(dialog->treeview), FALSE);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->treeview));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
  g_signal_connect(selection, "changed", G_CALLBACK(on_selection_changed),
                   dialog);

  /* CHECKBOX COLUMN */
  renderer = gtk_cell_renderer_toggle_new();
  column = gtk_tree_view_column_new_with_attributes(
      _("Enabled"), renderer, "active", STORE_COL_ENABLED, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->treeview), column);
  g_signal_connect(renderer, "toggled", G_CALLBACK(on_startup_enabled_toggled),
                   dialog);

  /* ICON COLUMN */
  renderer = gtk_cell_renderer_pixbuf_new();
  column = gtk_tree_view_column_new_with_attributes(
      _("Icon"), renderer, "gicon", STORE_COL_GICON, "sensitive",
      STORE_COL_ENABLED, NULL);
  g_object_set(renderer, "stock-size", GSM_PROPERTIES_ICON_SIZE, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->treeview), column);

  /* NAME COLUMN */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(
      _("Program"), renderer, "markup", STORE_COL_DESCRIPTION, "sensitive",
      STORE_COL_ENABLED, NULL);
  g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->treeview), column);

  gtk_tree_view_column_set_sort_column_id(column, STORE_COL_DESCRIPTION);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(dialog->treeview),
                                  STORE_COL_SEARCH);

  gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(dialog->treeview),
                                         GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                         NULL, 0, GDK_ACTION_COPY);
  gtk_drag_source_add_uri_targets(dialog->treeview);

  gtk_drag_dest_set(dialog->treeview, GTK_DEST_DEFAULT_ALL, NULL, 0,
                    GDK_ACTION_COPY);
  gtk_drag_dest_add_uri_targets(dialog->treeview);

  /* we don't want to accept drags coming from this widget */
  targetlist = gtk_drag_dest_get_target_list(dialog->treeview);
  if (targetlist != NULL) {
    GtkTargetEntry *targets;
    gint n_targets;
    gint i;

    targets = gtk_target_table_new_from_list(targetlist, &n_targets);
    for (i = 0; i < n_targets; i++) targets[i].flags = GTK_TARGET_OTHER_WIDGET;

    targetlist = gtk_target_list_new(targets, (guint)n_targets);
    gtk_drag_dest_set_target_list(dialog->treeview, targetlist);
    gtk_target_list_unref(targetlist);
    gtk_target_table_free(targets, n_targets);
  }

  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(dialog->list_store),
                                       STORE_COL_DESCRIPTION,
                                       GTK_SORT_ASCENDING);

  g_settings_bind(dialog->settings, SPC_AUTOSAVE_KEY, dialog->remember_toggle,
                  "active", G_SETTINGS_BIND_DEFAULT);

  dialog->manager = gsp_app_manager_get();
  gsp_app_manager_fill(dialog->manager);
  g_signal_connect_swapped(dialog->manager, "added", G_CALLBACK(_app_added),
                           dialog);
  g_signal_connect_swapped(dialog->manager, "removed", G_CALLBACK(_app_removed),
                           dialog);

  populate_model(dialog);
  gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(dialog->tree_filter));
}

static GObject *gsm_properties_dialog_constructor(
    GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties) {
  GsmPropertiesDialog *dialog;

  dialog = GSM_PROPERTIES_DIALOG(
      G_OBJECT_CLASS(gsm_properties_dialog_parent_class)
          ->constructor(type, n_construct_properties, construct_properties));

  setup_dialog(dialog);

  return G_OBJECT(dialog);
}

static void gsm_properties_dialog_dispose(GObject *object) {
  GsmPropertiesDialog *dialog;

  g_return_if_fail(object != NULL);
  g_return_if_fail(GSM_IS_PROPERTIES_DIALOG(object));

  dialog = GSM_PROPERTIES_DIALOG(object);

  g_clear_object(&dialog->builder);
  g_clear_object(&dialog->settings);

  G_OBJECT_CLASS(gsm_properties_dialog_parent_class)->dispose(object);

  /* it's important to do this after chaining to the parent dispose
   * method because we want to make sure the treeview has been disposed
   * and removed all its references to GspApp objects */
  g_clear_object(&dialog->manager);
}

static void gsm_properties_dialog_class_init(GsmPropertiesDialogClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  object_class->constructor = gsm_properties_dialog_constructor;
  object_class->dispose = gsm_properties_dialog_dispose;
  object_class->finalize = gsm_properties_dialog_finalize;

  gtk_widget_class_set_template_from_resource(
      widget_class, "/org/mate/session/properties.ui");
  gtk_widget_class_bind_template_child(widget_class, GsmPropertiesDialog,
                                       treeview);
  gtk_widget_class_bind_template_child(widget_class, GsmPropertiesDialog,
                                       notebook);
  gtk_widget_class_bind_template_child(widget_class, GsmPropertiesDialog,
                                       delete_button);
  gtk_widget_class_bind_template_child(widget_class, GsmPropertiesDialog,
                                       edit_button);
  gtk_widget_class_bind_template_child(widget_class, GsmPropertiesDialog,
                                       show_hidden_toggle);
  gtk_widget_class_bind_template_child(widget_class, GsmPropertiesDialog,
                                       remember_toggle);
  gtk_widget_class_bind_template_callback(widget_class, on_drag_begin);
  gtk_widget_class_bind_template_callback(widget_class, on_drag_data_get);
  gtk_widget_class_bind_template_callback(widget_class, on_drag_data_received);
  gtk_widget_class_bind_template_callback(widget_class, on_row_activated);
  gtk_widget_class_bind_template_callback(widget_class, on_add_app_clicked);
  gtk_widget_class_bind_template_callback(widget_class, on_delete_app_clicked);
  gtk_widget_class_bind_template_callback(widget_class, on_edit_app_clicked);
  gtk_widget_class_bind_template_callback(widget_class, on_show_hidden_toggled);
  gtk_widget_class_bind_template_callback(widget_class,
                                          on_save_session_clicked);
}

static void gsm_properties_dialog_init(GsmPropertiesDialog *dialog) {
  gtk_widget_init_template(GTK_WIDGET(dialog));

  gtk_widget_add_events(dialog->notebook, GDK_SCROLL_MASK);
  g_signal_connect(dialog->notebook, "scroll-event",
                   G_CALLBACK(on_main_notebook_scroll_event), NULL);
}

static void gsm_properties_dialog_finalize(GObject *object) {
  g_return_if_fail(object != NULL);
  g_return_if_fail(GSM_IS_PROPERTIES_DIALOG(object));

  G_OBJECT_CLASS(gsm_properties_dialog_parent_class)->finalize(object);
}

GsmPropertiesDialog *gsm_properties_dialog_new(void) {
  return g_object_new(GSM_TYPE_PROPERTIES_DIALOG, NULL);
}
