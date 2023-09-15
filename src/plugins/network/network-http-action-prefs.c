/* network-http-action-prefs.c
 *
 * Copyright 2023 Lorenzo Prosseda
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "network-http-action-prefs.h"

#include <glib/gi18n.h>

struct _NetworkHttpActionPrefs
{
  GtkBox parent_instance;

  GtkWidget *edit_row;
  GtkWidget *file_row;
  GtkLabel *filename_label;
  AdwComboRow *method_row;
  AdwComboRow *payload_src_row;
  GtkTextBuffer *payload_text_buffer;
  AdwPreferencesGroup *post_group;
  GtkWindow *text_edit_dialog;
  GtkEditable *uri_row;

  NetworkHttpAction *http_action;
};

G_DEFINE_FINAL_TYPE (NetworkHttpActionPrefs, network_http_action_prefs, GTK_TYPE_BOX)

enum
{
  PROP_0,
  PROP_ACTION,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS];


/*
 * Auxiliary methods
 */

static void
set_file (NetworkHttpActionPrefs *self,
          GFile                  *file)
{
  g_autofree char *basename = NULL;

  basename = g_file_get_basename (file);
  gtk_label_set_label (self->filename_label, basename);
  gtk_widget_set_tooltip_text (self->file_row, basename);

  network_http_action_set_file (self->http_action, file);
}

static void
update_payload_src_visibility (NetworkHttpActionPrefs *self)
{
  gtk_widget_set_visible (GTK_WIDGET (self->post_group),
                          adw_combo_row_get_selected (self->method_row) == METHOD_POST);

  gtk_widget_set_visible (self->file_row,
                          adw_combo_row_get_selected (self->payload_src_row) == SRC_FILE);
}


/*
 * Callbacks
 */

static void
on_uri_row_text_changed_cb (GtkEditable            *uri_row,
                            GParamSpec             *pspec,
                            NetworkHttpActionPrefs *self)
{
  network_http_action_set_uri (self->http_action, gtk_editable_get_text (uri_row));
}

static void
on_method_row_selected_changed_cb (AdwComboRow            *method_row,
                                   GParamSpec             *pspec,
                                   NetworkHttpActionPrefs *self)
{
  network_http_action_set_method (self->http_action,
                                  adw_combo_row_get_selected (method_row));

  update_payload_src_visibility (self);
}

static void
on_payload_src_row_selected_changed_cb (AdwComboRow            *payload_src_row,
                                        GParamSpec             *pspec,
                                        NetworkHttpActionPrefs *self)
{
  network_http_action_set_payload_src (self->http_action,
                                       adw_combo_row_get_selected (payload_src_row));

  gtk_widget_set_visible (self->file_row,
                          adw_combo_row_get_selected (self->payload_src_row) == SRC_FILE);
}

static void
on_file_dialog_response_cb (GObject      *source_object,
                            GAsyncResult *result,
                            gpointer      user_data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GFile) file = NULL;

  file = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source_object), result, &error);

  if (error)
    {
      g_warning ("Error opening file: %s\n", error->message);
      return;
    }

  set_file (NETWORK_HTTP_ACTION_PREFS (user_data), file);
}

static void
on_file_row_activated_cb (AdwActionRow           *row,
                          NetworkHttpActionPrefs *self)
{
  g_autoptr (GtkFileFilter) filter_text = NULL;
  g_autoptr (GtkFileFilter) filter_json = NULL;
  g_autoptr (GtkFileDialog) dialog = NULL;
  g_autoptr (GListStore) filters = NULL;

  dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _("Select payload file"));
  gtk_file_dialog_set_accept_label (dialog, _("Open"));
  gtk_file_dialog_set_modal (dialog, TRUE);

  filter_text = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_text, "Text Files");
  gtk_file_filter_add_mime_type (filter_text, "text/plain");

  filter_json = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_json, "JSON Files");
  gtk_file_filter_add_mime_type (filter_json, "application/json");

  filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (filters, filter_text);
  g_list_store_append (filters, filter_json);
  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));

  gtk_file_dialog_open (dialog,
                        GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))),
                        NULL,
                        on_file_dialog_response_cb,
                        self);
}

static void
opened_payload_file_cb (GObject      *source_object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  g_autoptr (GError) error = NULL;

  gtk_file_launcher_launch_finish (GTK_FILE_LAUNCHER (source_object),
                                   result,
                                   &error);

  if (error)
    g_warning ("Error launching app to edit file: %s\n", error->message);
}

static void
on_edit_row_activated_cb (AdwActionRow           *row,
                          NetworkHttpActionPrefs *self)
{
  g_assert (NETWORK_IS_HTTP_ACTION_PREFS (self));

  switch (adw_combo_row_get_selected (self->payload_src_row))
    {
    case SRC_TEXT:
      gtk_window_present (self->text_edit_dialog);
      break;

    case SRC_FILE:
      {
        g_autoptr (GtkFileLauncher) launcher = NULL;
        GFile *file;

        file = network_http_action_get_file (self->http_action);
        if (!file)
          return;

        launcher = gtk_file_launcher_new (file);
        gtk_file_launcher_launch (launcher,
                                  GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))),
                                  NULL,
                                  opened_payload_file_cb,
                                  self);
      }
      break;
    }

}

/*
 * GObject overrides
 */

static void
network_http_action_prefs_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  NetworkHttpActionPrefs *self = (NetworkHttpActionPrefs *)object;

  switch (prop_id)
    {
    case PROP_ACTION:
      g_value_set_object (value, self->http_action);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
network_http_action_prefs_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  NetworkHttpActionPrefs *self = (NetworkHttpActionPrefs *)object;

  switch (prop_id)
    {
    case PROP_ACTION:
      g_assert (self->http_action == NULL);
      g_set_weak_pointer (&self->http_action, g_value_get_object (value));
      g_assert (self->http_action != NULL);

      g_object_bind_property (self->payload_text_buffer,
                              "text",
                              self->http_action,
                              "payload-text",
                              G_BINDING_BIDIRECTIONAL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
network_http_action_prefs_class_init (NetworkHttpActionPrefsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = network_http_action_prefs_get_property;
  object_class->set_property = network_http_action_prefs_set_property;

  properties[PROP_ACTION] =
    g_param_spec_object ("action", NULL, NULL,
                         NETWORK_TYPE_HTTP_ACTION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/plugins/network/network-http-action-prefs.ui");

  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, edit_row);
  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, file_row);
  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, filename_label);
  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, method_row);
  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, payload_src_row);
  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, payload_text_buffer);
  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, post_group);
  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, text_edit_dialog);
  gtk_widget_class_bind_template_child (widget_class, NetworkHttpActionPrefs, uri_row);

  gtk_widget_class_bind_template_callback (widget_class, on_uri_row_text_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_method_row_selected_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_payload_src_row_selected_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_file_row_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_edit_row_activated_cb);
}

static void
network_http_action_prefs_init (NetworkHttpActionPrefs *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  update_payload_src_visibility (self);
}

GtkWidget *
network_http_action_prefs_new (NetworkHttpAction *http_action)
{
  return g_object_new (NETWORK_TYPE_HTTP_ACTION_PREFS,
                       "action", http_action,
                       NULL);
}

void
network_http_action_prefs_deserialize_settings (NetworkHttpActionPrefs *self,
                                                JsonObject             *settings)
{
  RequestMethod method;
  PayloadSource source;
  JsonNode *file_node = NULL;
  const char *payload_text;
  const char *uri;

  /* URI text field */
  uri = json_object_get_string_member_with_default (settings, "uri", "");
  gtk_editable_set_text (self->uri_row, uri);
  network_http_action_set_uri (self->http_action, uri);

  /* Method drop-down */
  method = json_object_get_int_member_with_default (settings, "method", METHOD_GET);
  adw_combo_row_set_selected (self->method_row, method);
  network_http_action_set_method (self->http_action, method);

  /* Payload source drop-down */
  source = json_object_get_int_member_with_default (settings, "source", SRC_TEXT);
  adw_combo_row_set_selected (self->payload_src_row, source);
  network_http_action_set_payload_src (self->http_action, source);

  /* Payload file chooser */
  file_node = json_object_get_member (settings, "payload_file");
  if (file_node && !JSON_NODE_HOLDS_NULL (file_node))
    {
      g_autoptr (GFile) file = g_file_new_for_path (json_node_get_string (file_node));
      set_file (self, file);
    }

  /* Payload text contents */
  payload_text = json_object_get_string_member_with_default (settings, "payload_text", NULL);
  network_http_action_set_payload_text (self->http_action, payload_text);
}

