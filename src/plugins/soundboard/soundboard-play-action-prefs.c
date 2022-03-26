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
on_file_chooser_native_response_cb (GtkNativeDialog           *native,
                                    int                        response,
                                    SoundboardPlayActionPrefs *self)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (native);
      g_autoptr (GFile) file = NULL;

      file = gtk_file_chooser_get_file (chooser);
      set_file (self, file);
    }

  g_object_unref (native);
}

static void
on_file_row_activated_cb (AdwActionRow              *row,
                          SoundboardPlayActionPrefs *self)
{
  g_autoptr (GtkFileFilter) filter = NULL;
  GtkFileChooserNative *native;

  native = gtk_file_chooser_native_new (_("Select audio file"),
                                        GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (self))),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        _("_Open"),
                                        _("_Cancel"));
  gtk_native_dialog_set_modal (GTK_NATIVE_DIALOG (native), TRUE);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Audio Files"));
  gtk_file_filter_add_mime_type (filter, "audio/*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (native), filter);

  g_signal_connect (native, "response", G_CALLBACK (on_file_chooser_native_response_cb), self);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (native));
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
soundboard_play_action_prefs_class_init (SoundboardPlayActionPrefsClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

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
