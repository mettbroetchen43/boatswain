/* launcher-launch-action.h
 *
 * Copyright 2022 Emmanuele Bassi
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

#include "launcher-launch-preferences.h"

#include <glib/gi18n.h>

struct _LauncherLaunchPreferences
{
  AdwPreferencesGroup parent_instance;

  GtkAppChooserButton *app_chooser;
  GtkLabel *app_name;

  LauncherLaunchAction *launch_action;
  GAppInfo *app_info;
};

G_DEFINE_FINAL_TYPE (LauncherLaunchPreferences, launcher_launch_preferences, ADW_TYPE_PREFERENCES_GROUP)


/*
 * Auxiliary methods
 */

static void
update_app_info (LauncherLaunchPreferences *self)
{
  gtk_widget_set_visible (GTK_WIDGET (self->app_name), self->app_info != NULL);

  if (self->app_info)
    {
      const char *display_name = g_app_info_get_display_name (self->app_info);

      gtk_label_set_text (self->app_name, display_name);
      gtk_widget_set_visible (GTK_WIDGET (self->app_name), TRUE);
    }
  else
    {
      gtk_label_set_text (self->app_name, "Unknown application");
    }
}


/*
 * Callbacks
 */

static void
on_app_chooser_response (GtkDialog                 *dialog,
                         int                        response_id,
                         LauncherLaunchPreferences *self)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      g_autoptr (GAppInfo) app_info = NULL;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      app_info = gtk_app_chooser_get_app_info (GTK_APP_CHOOSER (dialog));
G_GNUC_END_IGNORE_DEPRECATIONS

      launcher_launch_action_set_app (self->launch_action, app_info);

      g_set_object (&self->app_info, app_info);
      update_app_info (self);
    }

  gtk_window_close (GTK_WINDOW (dialog));
}

static void
on_app_row_activated_cb (LauncherLaunchPreferences *self)
{
  GtkAppChooserDialog *dialog;
  GtkWidget *window;

  window = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW);
  dialog = g_object_new (GTK_TYPE_APP_CHOOSER_DIALOG,
                         "transient-for", window,
                         "modal", TRUE,
                         "destroy-with-parent", TRUE,
                         NULL);

  g_signal_connect (dialog, "response", G_CALLBACK (on_app_chooser_response), self);

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
launcher_launch_preferences_class_init (LauncherLaunchPreferencesClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/plugins/launcher/launcher-launch-preferences.ui");

  gtk_widget_class_bind_template_child (widget_class, LauncherLaunchPreferences, app_name);

  gtk_widget_class_bind_template_callback (widget_class, on_app_row_activated_cb);
}

static void
launcher_launch_preferences_init (LauncherLaunchPreferences *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
launcher_launch_preferences_new (LauncherLaunchAction *launch_action)
{
  LauncherLaunchPreferences *self = g_object_new (LAUNCHER_TYPE_LAUNCH_PREFERENCES, NULL);

  self->launch_action = launch_action;

  return GTK_WIDGET (self);
}

void
launcher_launch_preferences_update (LauncherLaunchPreferences *self,
                                    GAppInfo                  *app_info,
                                    GList                     *files)
{
  g_return_if_fail (LAUNCHER_IS_LAUNCH_PREFERENCES (self));

  g_set_object (&self->app_info, app_info);
  update_app_info (self);
}
