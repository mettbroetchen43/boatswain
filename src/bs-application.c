/* bs-application.c
 *
 * Copyright 2022 Georges Basile Stavracas Neto
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
 */

#include "bs-action-factory.h"
#include "bs-application.h"
#include "bs-device-manager.h"
#include "bs-window.h"

#include <glib/gi18n.h>
#include <libportal/portal.h>
#include <libportal-gtk4/portal-gtk4.h>

struct _BsApplication
{
  AdwApplication parent_instance;

  GtkWindow *window;

  PeasExtensionSet *action_factories_set;
  BsDeviceManager *device_manager;
  XdpPortal *portal;
  guint request_background_timeout_id;
};

static void on_request_background_called_cb (GObject      *object,
                                             GAsyncResult *result,
                                             gpointer      user_data);

G_DEFINE_TYPE (BsApplication, bs_application, ADW_TYPE_APPLICATION)


/*
 * Auxiliary methods
 */

static void
request_run_in_background (BsApplication *self)
{
  g_autoptr (XdpParent) parent = NULL;
  XdpBackgroundFlags background_flags;

  if (self->window)
    parent = xdp_parent_new_gtk (self->window);

  background_flags = XDP_BACKGROUND_FLAG_ACTIVATABLE;

  xdp_portal_request_background (self->portal,
                                 parent,
                                 _("Boatswain needs to run in background to detect and execute Stream Deck actions."),
                                 NULL,
                                 background_flags,
                                 NULL,
                                 on_request_background_called_cb,
                                 self);
}


/*
 * Callbacks
 */

static void
on_request_background_called_cb (GObject      *object,
                                 GAsyncResult *result,
                                 gpointer      user_data)
{
  BsApplication *self = BS_APPLICATION (user_data);
  g_autoptr (GError) error = NULL;

  xdp_portal_request_background_finish (XDP_PORTAL (object), result, &error);

  if (error)
    {
      g_warning ("Error requesting background: %s", error->message);
      return;
    }

  /* Hold the application is we're allowed to run in background */
  g_application_hold (G_APPLICATION (self));
}

static void
bs_application_show_about (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
  BsApplication *self = BS_APPLICATION (user_data);
  GtkWindow *window = NULL;
  const char *authors[] = {
    "Georges Basile Stavracas Neto <georges.stavracas@gmail.com>",
    NULL,
  };

  g_return_if_fail (BS_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  gtk_show_about_dialog (window,
                         "program-name", "Boatswain",
                         "authors", authors,
                         "version", "0.1.0",
                         NULL);
}

static gboolean
request_background_cb (gpointer user_data)
{
  BsApplication *self = BS_APPLICATION (user_data);

  if (!self->window ||
      !gtk_widget_get_mapped (GTK_WIDGET (self->window)) ||
      !gtk_widget_get_realized (GTK_WIDGET (self->window)) ||
      !gtk_window_is_active (self->window))
    return G_SOURCE_CONTINUE;

  request_run_in_background (self);

  self->request_background_timeout_id = 0;
  return G_SOURCE_REMOVE;
}


/*
 * GApplication overrides
 */

static void
bs_application_startup (GApplication *application)
{
  BsApplication *self = BS_APPLICATION (application);
  PeasEngine *engine;
  const GList *l;

  G_APPLICATION_CLASS (bs_application_parent_class)->startup (application);

  /* All plugins must be loaded before profiles and Stream Decks */
  engine = peas_engine_get_default ();
  peas_engine_prepend_search_path (engine,
                                   "resource:///com/feaneron/Boatswain/plugins",
                                   "resource:///com/feaneron/Boatswain/plugins");
  for (l = peas_engine_get_plugin_list (engine); l; l = l->next)
    peas_engine_load_plugin (engine, l->data);

  self->action_factories_set = peas_extension_set_new (peas_engine_get_default (),
                                                       BS_TYPE_ACTION_FACTORY,
                                                       NULL);

  self->device_manager = bs_device_manager_new ();
  self->portal = xdp_portal_new ();

  self->request_background_timeout_id = g_timeout_add_seconds (1, request_background_cb, self);
}

static void
bs_application_activate (GApplication *application)
{
  BsApplication *self = BS_APPLICATION (application);

  if (!self->window)
    {
      self->window = g_object_new (BS_TYPE_WINDOW,
                                   "application", application,
                                   NULL);
      g_object_add_weak_pointer (G_OBJECT (self->window), (gpointer*) &self->window);
    }

  gtk_window_present (self->window);
}

static void
bs_application_shutdown (GApplication *application)
{
  BsApplication *self = BS_APPLICATION (application);

  g_clear_handle_id (&self->request_background_timeout_id, g_source_remove);
  g_clear_pointer (&self->window, gtk_window_destroy);
  g_clear_object (&self->device_manager);
  g_clear_object (&self->portal);

  G_APPLICATION_CLASS (bs_application_parent_class)->shutdown (application);
}


/*
 * GObject overrides
 */

static void
bs_application_finalize (GObject *object)
{
  BsApplication *self = (BsApplication *)object;

  g_clear_object (&self->device_manager);
  g_clear_object (&self->portal);

  G_OBJECT_CLASS (bs_application_parent_class)->finalize (object);
}

static void
bs_application_class_init (BsApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = bs_application_finalize;

  app_class->startup = bs_application_startup;
  app_class->activate = bs_application_activate;
  app_class->shutdown = bs_application_shutdown;
}

static void
bs_application_init (BsApplication *self)
{
  g_autoptr (GSimpleAction) quit_action = g_simple_action_new ("quit", NULL);
  g_signal_connect_swapped (quit_action, "activate", G_CALLBACK (g_application_quit), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (quit_action));

  g_autoptr (GSimpleAction) about_action = g_simple_action_new ("about", NULL);
  g_signal_connect (about_action, "activate", G_CALLBACK (bs_application_show_about), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (about_action));

  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "app.quit",
                                         (const char *[]) {
                                           "<primary>q",
                                           NULL,
                                         });
}

BsApplication *
bs_application_new (void)
{
  return g_object_new (BS_TYPE_APPLICATION,
                       "application-id", "com.feaneron.Boatswain",
                       "flags", G_APPLICATION_FLAGS_NONE,
                       NULL);
}

BsDeviceManager *
bs_application_get_device_manager (BsApplication *self)
{
  g_return_val_if_fail (BS_IS_APPLICATION (self), NULL);

  return self->device_manager;
}

PeasExtensionSet *
bs_application_get_action_factory_set (BsApplication *self)
{
  g_return_val_if_fail (BS_IS_APPLICATION (self), NULL);

  return self->action_factories_set;
}
