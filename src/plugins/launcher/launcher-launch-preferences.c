#include "launcher-launch-preferences.h"

#include <glib/gi18n.h>

struct _LauncherLaunchPreferences
{
  AdwPreferencesGroup parent_instance;

  GtkAppChooserButton *app_chooser;
  GtkLabel *app_name;

  LauncherLaunchAction *launch_action;
};

G_DEFINE_FINAL_TYPE (LauncherLaunchPreferences, launcher_launch_preferences, ADW_TYPE_PREFERENCES_GROUP)

static void
on_app_chooser_response (GtkDialog                 *dialog,
                         int                        response_id,
                         LauncherLaunchPreferences *self)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GAppInfo *app_info = gtk_app_chooser_get_app_info (GTK_APP_CHOOSER (dialog));

      launcher_launch_action_set_app (self->launch_action, app_info);

      g_print ("Found app info: %s [%p]\n",
               g_app_info_get_name (app_info),
               app_info);

      if (app_info)
        {
          const char *display_name = g_app_info_get_display_name (app_info);

          gtk_label_set_text (GTK_LABEL (self->app_name), display_name);
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (self->app_name), "Unknown application");
        }
    }

  gtk_window_close (GTK_WINDOW (dialog));
}

static void
on_app_row_activated_cb (LauncherLaunchPreferences *self)
{
  GtkWidget *window = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW);

  GtkAppChooserDialog *dialog = g_object_new (GTK_TYPE_APP_CHOOSER_DIALOG, NULL);

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

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

}
