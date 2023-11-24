/* soundboard-play-action-prefs.c
 *
 * Copyright 2022 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#include "soundboard-play-action-prefs.h"

#include <glib/gi18n.h>

struct _SoundboardPlayActionPrefs
{
  AdwPreferencesGroup parent_instance;

  AdwComboRow *behavior_row;
  GtkWidget *file_row;
  GtkLabel *filename_label;
  GtkAdjustment *volume_adjustment;

  SoundboardPlayAction *play_action;

  GCancellable *cancellable;
};

G_DEFINE_FINAL_TYPE (SoundboardPlayActionPrefs, soundboard_play_action_prefs, ADW_TYPE_PREFERENCES_GROUP)


/*
 * Auxiliary methods
 */

static void
set_file (SoundboardPlayActionPrefs *self,
          GFile                     *file)
{
  g_autofree char *basename = NULL;

  basename = g_file_get_basename (file);
  gtk_label_set_label (self->filename_label, basename);
  gtk_widget_set_tooltip_text (self->file_row, basename);

  soundboard_play_action_set_file (self->play_action, file);
}


/*
 * Callbacks
 */

static void
on_behavior_row_selected_changed_cb (AdwComboRow               *behavior_row,
                                     GParamSpec                *pspec,
                                     SoundboardPlayActionPrefs *self)
{
  soundboard_play_action_set_behavior (self->play_action,
                                       adw_combo_row_get_selected (behavior_row));
}

static void
on_file_dialog_opened_cb (GObject      *source,
                          GAsyncResult *result,
                          gpointer      user_data)
{
  SoundboardPlayActionPrefs *self = SOUNDBOARD_PLAY_ACTION_PREFS (user_data);
  g_autoptr (GError) error = NULL;
  g_autoptr (GFile) file = NULL;

  file = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source), result, &error);
  if (file)
    set_file (self, file);
}

static void
on_file_row_activated_cb (AdwActionRow              *row,
                          SoundboardPlayActionPrefs *self)
{
  g_autoptr (GtkFileDialog) file_dialog = NULL;
  g_autoptr (GtkFileFilter) filter = NULL;
  g_autoptr (GListStore) filters = NULL;

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Audio Files"));
  gtk_file_filter_add_mime_type (filter, "audio/*");

  filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (filters, filter);

  file_dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (file_dialog, _("Select audio file"));
  gtk_file_dialog_set_modal (file_dialog, TRUE);
  gtk_file_dialog_set_filters (file_dialog, G_LIST_MODEL (filters));

  gtk_file_dialog_open (file_dialog,
                        GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))),
                        self->cancellable,
                        on_file_dialog_opened_cb,
                        self);
}

static void
on_volume_adjustment_value_changed_cb (GtkAdjustment             *adjustment,
                                       GParamSpec                *pspec,
                                       SoundboardPlayActionPrefs *self)
{
  soundboard_play_action_set_volume (self->play_action, gtk_adjustment_get_value (adjustment));
}


/*
 * GObject overrides
 */

static void
soundboard_play_action_prefs_dispose (GObject *object)
{
  SoundboardPlayActionPrefs *self = (SoundboardPlayActionPrefs *)object;

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);

  G_OBJECT_CLASS (soundboard_play_action_prefs_parent_class)->dispose (object);
}

static void
soundboard_play_action_prefs_class_init (SoundboardPlayActionPrefsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = soundboard_play_action_prefs_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/plugins/soundboard/soundboard-play-action-prefs.ui");

  gtk_widget_class_bind_template_child (widget_class, SoundboardPlayActionPrefs, behavior_row);
  gtk_widget_class_bind_template_child (widget_class, SoundboardPlayActionPrefs, file_row);
  gtk_widget_class_bind_template_child (widget_class, SoundboardPlayActionPrefs, filename_label);
  gtk_widget_class_bind_template_child (widget_class, SoundboardPlayActionPrefs, volume_adjustment);

  gtk_widget_class_bind_template_callback (widget_class, on_behavior_row_selected_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_file_row_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_volume_adjustment_value_changed_cb);
}

static void
soundboard_play_action_prefs_init (SoundboardPlayActionPrefs *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->cancellable = g_cancellable_new ();
}

GtkWidget *
soundboard_play_action_prefs_new (SoundboardPlayAction *play_action)
{
  SoundboardPlayActionPrefs *self;

  self = g_object_new (SOUNDBOARD_TYPE_PLAY_ACTION_PREFS, NULL);
  self->play_action = play_action;

  return GTK_WIDGET (self);
}

void
soundboard_play_action_prefs_deserialize_settings (SoundboardPlayActionPrefs *self,
                                                   JsonObject                *settings)
{
  SoundboardPlayBehavior behavior;
  JsonNode *file_node = NULL;
  double volume;

  file_node = json_object_get_member (settings, "file");
  if (file_node && !JSON_NODE_HOLDS_NULL (file_node))
    {
      g_autoptr (GFile) file = NULL;

      file = g_file_new_for_path (json_node_get_string (file_node));
      set_file (self, file);
    }

  behavior = json_object_get_int_member (settings, "behavior");
  adw_combo_row_set_selected (self->behavior_row, behavior);
  soundboard_play_action_set_behavior (self->play_action, behavior);

  volume = json_object_get_double_member (settings, "volume");
  gtk_adjustment_set_value (self->volume_adjustment, volume);
  soundboard_play_action_set_volume (self->play_action, volume);
}
