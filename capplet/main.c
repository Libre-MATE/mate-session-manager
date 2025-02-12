/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 * main.c
 * Copyright (C) 1999 Free Software Foundation, Inc.
 * Copyright (C) 2008 Lucas Rocha.
 * Copyright (C) 2012-2021 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>

#include "gsm-properties-dialog.h"

static gboolean show_version = FALSE;

static GOptionEntry options[] = {
    {"version", 0, 0, G_OPTION_ARG_NONE, &show_version,
     N_("Version of this application"), NULL},
    {NULL, 0, 0, 0, NULL, NULL, NULL}};

static void dialog_response(GsmPropertiesDialog* dialog, gint response_id,
                            gpointer data) {
  GError* error;

  if (response_id == GTK_RESPONSE_HELP) {
    error = NULL;
    gtk_show_uri_on_window(GTK_WINDOW(dialog),
                           "help:mate-user-guide/gosstartsession-2",
                           gtk_get_current_event_time(), &error);

    if (error != NULL) {
      GtkWidget* d = gtk_message_dialog_new(
          GTK_WINDOW(dialog), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s",
          _("Could not display help document"));
      gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(d), "%s",
                                               error->message);
      g_error_free(error);

      gtk_dialog_run(GTK_DIALOG(d));
      gtk_widget_destroy(d);
    }
  } else {
    gtk_widget_destroy(GTK_WIDGET(dialog));
    gtk_main_quit();
  }
}

int main(int argc, char *argv[]) {
  GError *error;
  GsmPropertiesDialog *dialog;

#ifdef ENABLE_NLS
  bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

  error = NULL;

  if (!gtk_init_with_args(&argc, &argv, _("- MATE Session Properties"), options,
                          GETTEXT_PACKAGE, &error)) {
    g_warning("Unable to start: %s", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  if (show_version) {
    g_print("%s %s\n", argv[0], VERSION);
    return EXIT_SUCCESS;
  }

  dialog = gsm_properties_dialog_new();
  g_signal_connect(dialog, "response", G_CALLBACK(dialog_response), NULL);
  gtk_widget_show(GTK_WIDGET(dialog));

  gtk_main();

  return EXIT_SUCCESS;
}
