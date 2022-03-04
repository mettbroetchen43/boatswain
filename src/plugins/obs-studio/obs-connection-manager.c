/* obs-connection-manager.c
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

#include "obs-connection-manager.h"

struct _ObsConnectionManager
{
  GObject parent_instance;

  GHashTable *connections;
};

G_DEFINE_FINAL_TYPE (ObsConnectionManager, obs_connection_manager, G_TYPE_OBJECT)


/*
 * Callbacks
 */

static void
remove_connection_cb (gpointer  data,
                      GObject  *object,
                      gboolean  is_last_ref)
{
  ObsConnectionManager *self = OBS_CONNECTION_MANAGER (data);

  if (is_last_ref)
    {
      g_autofree char *key = NULL;

      key = g_strdup_printf ("%s:::%u",
                             obs_connection_get_host (OBS_CONNECTION (object)),
                             obs_connection_get_port (OBS_CONNECTION (object)));

      g_hash_table_remove (self->connections, key);
    }
}


/*
 * GObject overrides
 */

static void
obs_connection_manager_finalize (GObject *object)
{
  ObsConnectionManager *self = (ObsConnectionManager *)object;

  g_clear_pointer (&self->connections, g_hash_table_destroy);

  G_OBJECT_CLASS (obs_connection_manager_parent_class)->finalize (object);
}

static void
obs_connection_manager_class_init (ObsConnectionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = obs_connection_manager_finalize;
}

static void
obs_connection_manager_init (ObsConnectionManager *self)
{
  self->connections = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

ObsConnectionManager *
obs_connection_manager_new (void)
{
  return g_object_new (OBS_TYPE_CONNECTION_MANAGER, NULL);
}

ObsConnection *
obs_connection_manager_get (ObsConnectionManager  *self,
                            const char            *host,
                            unsigned int           port,
                            GError               **error)
{
  g_autoptr (ObsConnection) connection = NULL;
  g_autofree char *parsed_uri_string = NULL;

  g_return_val_if_fail (OBS_IS_CONNECTION_MANAGER (self), NULL);
  g_return_val_if_fail (host != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  parsed_uri_string = g_strdup_printf ("%s:::%u", host, port);
  connection = g_hash_table_lookup (self->connections, parsed_uri_string);
  if (!connection)
    {
      connection = obs_connection_new (host, port);
      g_hash_table_insert (self->connections, g_steal_pointer (&parsed_uri_string), connection);
      g_object_add_toggle_ref (G_OBJECT (connection), remove_connection_cb, self);
    }
  else
    {
      g_object_ref (connection);
    }

  return g_steal_pointer (&connection);
}
