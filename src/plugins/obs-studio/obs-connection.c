/* obs-connection.c
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

#include "obs-connection.h"

#include <libsecret/secret.h>
#include <glib/gi18n.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <stdint.h>

struct _ObsConnection
{
  GObject parent_instance;

  char *host;
  unsigned int port;
  ObsConnectionState state;

  GListStore *scenes;

  struct {
    char *challenge;
    char *salt;
  } authentication;
  guint reconnect_timeout_id;

  GHashTable *uuid_to_task;
  GCancellable *cancellable;

  gboolean streaming;
  ObsRecordingState recording_state;

  SoupSession *session;
	SoupWebsocketConnection *websocket_client;
};

static int compare_scenes_by_name_cb (gconstpointer a,
                                      gconstpointer b,
                                      gpointer      user_data);

static void on_websocket_get_scene_list_cb (GObject      *source_object,
                                            GAsyncResult *result,
                                            gpointer      user_data);

static void websocket_connected_cb (GObject      *source_object,
                                    GAsyncResult *result,
                                    gpointer      user_data);

G_DEFINE_FINAL_TYPE (ObsConnection, obs_connection, G_TYPE_OBJECT)

enum
{
  PROP_0,
  PROP_HOST,
  PROP_PORT,
  PROP_RECORDING_STATE,
  PROP_STREAMING,
  N_PROPS,
};

enum
{
  STATE_CHANGED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];
static GParamSpec *properties[N_PROPS];

/*
 * Auxiliary methods
 */

static SecretSchema secrets_schema = {
  "com.feaneron.Boatswain.plugin.obs-studio",
  SECRET_SCHEMA_NONE,
  {
    { "host", SECRET_SCHEMA_ATTRIBUTE_STRING },
    { "port", SECRET_SCHEMA_ATTRIBUTE_INTEGER },
    { "NULL", 0 },
  },
};

static char *
lookup_password (ObsConnection *self)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *password = NULL;

  password = secret_password_lookup_sync (&secrets_schema,
                                          self->cancellable,
                                          &error,
                                          "host", self->host,
                                          "port", self->port,
                                          NULL);

  if (error)
    g_warning ("Error fetching password: %s", error->message);

  return g_steal_pointer (&password);
}


static void
save_password (ObsConnection *self,
               const char    *password)
{
  secret_password_store (&secrets_schema,
                         SECRET_COLLECTION_SESSION,
                         "obs-websocket connection password",
                         password,
                         NULL, NULL, NULL,
                         "host", self->host,
                         "port", self->port,
                         NULL);
}

static void
set_recording_state (ObsConnection     *self,
                     ObsRecordingState  recording_state)
{
  if (self->recording_state == recording_state)
    return;

  self->recording_state = recording_state;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_RECORDING_STATE]);
}

static void
set_streaming (ObsConnection *self,
               gboolean       streaming)
{
  if (self->streaming == streaming)
    return;

  self->streaming = streaming;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_STREAMING]);
}

static char *
generate_auth_string (const char *password,
                      const char *challenge,
                      const char *salt)
{
  g_autoptr (GChecksum) secret_checksum = NULL;
  g_autoptr (GChecksum) auth_checksum = NULL;
  g_autoptr (GString) secret_string = NULL;
  g_autoptr (GString) auth_string = NULL;
  g_autofree uint8_t *secret_hash = NULL;
  g_autofree uint8_t *auth_hash = NULL;
  g_autofree char *secret = NULL;
  g_autofree char *auth = NULL;
  size_t secret_length;
  size_t auth_length;

  secret_string = g_string_new (password);
  g_string_append (secret_string, salt);

  secret_checksum = g_checksum_new (G_CHECKSUM_SHA256);
  g_checksum_update (secret_checksum, (guchar *)secret_string->str, secret_string->len);

  secret_length = 200;
  secret_hash = g_malloc0 (sizeof (uint8_t) * secret_length);
  g_checksum_get_digest (secret_checksum, secret_hash, &secret_length);
  secret = g_base64_encode (secret_hash, secret_length);

  auth_string = g_string_new (secret);
  g_string_append (auth_string, challenge);

  auth_checksum = g_checksum_new (G_CHECKSUM_SHA256);
  g_checksum_update (auth_checksum, (guchar *)auth_string->str, auth_string->len);

  auth_length = 200;
  auth_hash = g_malloc0 (sizeof (uint8_t) * auth_length);
  g_checksum_get_digest (auth_checksum, auth_hash, &auth_length);
  auth = g_base64_encode (auth_hash, auth_length);

  return g_steal_pointer (&auth);
}

static void
set_connection_state (ObsConnection      *self,
                      ObsConnectionState  state)
{
  ObsConnectionState old_state;

  if (self->state == state)
    return;

  old_state = self->state;
  self->state = state;

  if (state != OBS_CONNECTION_STATE_CONNECTED)
    {
      g_list_store_remove_all (self->scenes);
      set_recording_state (self, OBS_RECORDING_STATE_STOPPED);
      set_streaming (self, FALSE);
    }

  g_signal_emit (self, signals[STATE_CHANGED], 0, old_state, state);
}

static void
send_message (ObsConnection       *self,
              JsonBuilder         *builder,
              GCancellable        *cancellable,
              GAsyncReadyCallback  callback,
              gpointer             user_data)
{
  g_autoptr (JsonGenerator) generator = NULL;
  g_autoptr (GTask) task = NULL;
  g_autofree char *message = NULL;
  g_autofree char *uuid = NULL;

  uuid = g_uuid_string_random ();
  json_builder_set_member_name (builder, "message-id");
  json_builder_add_string_value (builder, uuid);
  json_builder_end_object (builder);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, g_strdup (uuid), g_free);
  g_hash_table_insert (self->uuid_to_task, g_steal_pointer (&uuid), g_steal_pointer (&task));

  generator = json_generator_new ();
  json_generator_set_root (generator, json_builder_get_root (builder));

  message = json_generator_to_data (generator, NULL);
  soup_websocket_connection_send_text (self->websocket_client, message);
}

static GBytes *
send_message_finish (GAsyncResult  *result,
                     GError       **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

static inline JsonNode *
parse_message_response (GAsyncResult  *result,
                        GError       **error)
{
  g_autoptr (JsonParser) parser = NULL;
  g_autoptr (JsonNode) root = NULL;
  g_autoptr (GBytes) message = NULL;
  const char *data;
  size_t length;

  message = send_message_finish (result, error);

  if (!message)
    return NULL;

  data = g_bytes_get_data (message, &length);
  parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, data, length, error))
    return NULL;

  root = json_parser_steal_root (parser);

  if (!JSON_NODE_HOLDS_OBJECT (root))
    {
      g_set_error (error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_UNKNOWN, "Invalid response");
      return NULL;
    }

  return g_steal_pointer (&root);
}

static void
connect_to_obs_websocket (ObsConnection *self)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autofree char *address = NULL;

  g_assert (self->state == OBS_CONNECTION_STATE_DISCONNECTED);

  address = g_strdup_printf ("%s:%u", self->host, self->port);
  message = soup_message_new (SOUP_METHOD_GET, address);

  soup_session_websocket_connect_async (self->session,
                                        message,
                                        NULL,
                                        NULL,
                                        G_PRIORITY_DEFAULT,
                                        self->cancellable,
                                        websocket_connected_cb,
                                        self);

  set_connection_state (self, OBS_CONNECTION_STATE_CONNECTING);
}

static void
fetch_scenes (ObsConnection *self)
{
  g_autoptr (JsonBuilder) builder = NULL;

  /* Check if the connection needs authentication */
  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "request-type");
  json_builder_add_string_value (builder, "GetSceneList");

  send_message (self, builder, self->cancellable, on_websocket_get_scene_list_cb, self);
}

static gboolean
reconnect_after_timeout_cb (gpointer data)
{
  ObsConnection *self = OBS_CONNECTION (data);

  connect_to_obs_websocket (self);

  self->reconnect_timeout_id = 0;
  return G_SOURCE_REMOVE;
}

static void
reconnect_after_timeout (ObsConnection *self)
{
  if (self->reconnect_timeout_id > 0)
    return;

  self->reconnect_timeout_id = g_timeout_add_seconds (1, reconnect_after_timeout_cb, self);
}


/*
 * Websocket events
 */

static void
recording_paused_cb (ObsConnection *self,
                     JsonObject    *object)
{
  set_recording_state (self, OBS_RECORDING_STATE_PAUSED);
}

static void
recording_resumed_cb (ObsConnection *self,
                      JsonObject    *object)
{
  set_recording_state (self, OBS_RECORDING_STATE_RECORDING);
}

static void
recording_started_cb (ObsConnection *self,
                      JsonObject    *object)
{
  set_recording_state (self, OBS_RECORDING_STATE_RECORDING);
}

static void
recording_stopped_cb (ObsConnection *self,
                      JsonObject    *object)
{
  set_recording_state (self, OBS_RECORDING_STATE_STOPPED);
}

static void
scenes_changed_cb (ObsConnection *self,
                   JsonObject    *object)
{
  g_autoptr (GHashTable) new_scenes = NULL;
  JsonNode *scenes_node;
  unsigned int i;

  scenes_node = json_object_get_member (object, "scenes");
  new_scenes = g_hash_table_new (g_str_hash, g_str_equal);

  if (JSON_NODE_HOLDS_ARRAY (scenes_node))
    {
      JsonArray *scenes_array;
      unsigned int i;

      scenes_array = json_node_get_array (scenes_node);
      for (i = 0; i < json_array_get_length (scenes_array); i++)
        {
          JsonObject *scene_object;

          scene_object = json_array_get_object_element (scenes_array, i);
          g_hash_table_add (new_scenes, (gpointer) json_object_get_string_member (scene_object, "name"));
        }
    }

  for (i = g_list_model_get_n_items (G_LIST_MODEL (self->scenes)); i > 0; i--)
    {
      g_autoptr (ObsScene) scene = g_list_model_get_item (G_LIST_MODEL (self->scenes), i - 1);
      const char *scene_name = obs_scene_get_name (scene);

      if (g_hash_table_contains (new_scenes, (gpointer) scene_name))
        g_hash_table_remove (new_scenes, (gpointer) scene_name);
      else
        g_list_store_remove (self->scenes, i - 1);
    }
}

static void
source_create_event_cb (ObsConnection *self,
                        JsonObject    *object)
{
  g_autoptr (ObsScene) scene = NULL;
  const char *source_kind;

  source_kind = json_object_get_string_member_with_default (object, "sourceType", NULL);

  if (g_strcmp0 (source_kind, "scene") != 0)
    return;

  scene = obs_scene_new (json_object_get_string_member (object, "sourceName"));
  g_list_store_insert_sorted (self->scenes, scene, compare_scenes_by_name_cb, NULL);
}

static void
source_renamed_event_cb (ObsConnection *self,
                         JsonObject    *object)
{
  unsigned int i;
  const char *previous_name;
  const char *source_kind;

  source_kind = json_object_get_string_member_with_default (object, "sourceType", NULL);

  if (g_strcmp0 (source_kind, "scene") != 0)
    return;

  previous_name = json_object_get_string_member (object, "previousName");

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->scenes)); i++)
    {
      g_autoptr (ObsScene) scene = g_list_model_get_item (G_LIST_MODEL (self->scenes), i);

      if (g_strcmp0 (obs_scene_get_name (scene), previous_name) == 0)
        {
          obs_scene_set_name (scene, json_object_get_string_member (object, "newName"));
          g_list_store_sort (self->scenes, compare_scenes_by_name_cb, NULL);
          break;
        }
    }
}

static void
stream_started_cb (ObsConnection *self,
                   JsonObject    *object)
{
  set_streaming (self, TRUE);
}

static void
stream_stopped_cb (ObsConnection *self,
                   JsonObject    *object)
{
  set_streaming (self, FALSE);
}

struct {
  const char *event_name;
  void (*trigger) (ObsConnection *self,
                   JsonObject    *object);
} events_vtable[] = {
  { "RecordingPaused", recording_paused_cb },
  { "RecordingResumed", recording_resumed_cb },
  { "RecordingStarted", recording_started_cb },
  { "RecordingStopped", recording_stopped_cb },
  { "ScenesChanged", scenes_changed_cb },
  { "SourceCreated", source_create_event_cb },
  { "SourceRenamed", source_renamed_event_cb },
  { "StreamStarted", stream_started_cb },
  { "StreamStopped", stream_stopped_cb },
};

static void
parse_event (ObsConnection *self,
             JsonObject    *object)
{
  const char *update_type;
  size_t i;

  update_type = json_object_get_string_member_with_default (object, "update-type", NULL);

  for (i = 0; i < G_N_ELEMENTS (events_vtable); i++)
    {
      if (g_strcmp0 (update_type, events_vtable[i].event_name) == 0)
        {
          events_vtable[i].trigger (self, object);
          break;
        }
    }
}


/*
 * Callbacks
 */

static int
compare_scenes_by_name_cb (gconstpointer a,
                           gconstpointer b,
                           gpointer      user_data)
{
  ObsScene *scene_a = OBS_SCENE ((gpointer) a);
  ObsScene *scene_b = OBS_SCENE ((gpointer) b);

  return g_strcmp0 (obs_scene_get_name (scene_a), obs_scene_get_name (scene_b));
}

static void
on_connection_authenticated_cb (GObject      *source_object,
                                GAsyncResult *result,
                                gpointer      user_data)
{
  g_autoptr (GError) error = NULL;

  obs_connection_authenticate_finish (OBS_CONNECTION (source_object), result, &error);

  if (error)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_PROXY_AUTH_FAILED))
        g_warning ("Error authenticating connection: %s", error->message);
    }
}

static void
on_websocket_client_closed_cb (SoupWebsocketConnection *websocket_client,
                               ObsConnection           *self)
{
  set_connection_state (self, OBS_CONNECTION_STATE_DISCONNECTED);
  reconnect_after_timeout (self);
}

static void
on_websocket_client_error_cb (SoupWebsocketConnection *websocket_client,
                              ObsConnection           *self)
{
  g_message ("Websocket error");
}

static void
on_websocket_client_message_cb (SoupWebsocketConnection *websocket_client,
                                int                      type,
                                GBytes                  *message,
                                ObsConnection           *self)
{
  g_autoptr (JsonParser) parser = NULL;
  g_autoptr (GError) error = NULL;
  JsonObject *root_object;
  const char *data;
  const char *uuid;
  size_t length;

  data = g_bytes_get_data (message, &length);

  // Useful for debugging:
  // g_message ("Message received: %s", data);

  parser = json_parser_new ();
  json_parser_load_from_data (parser, data, length, &error);

  if (error)
    {
      g_warning ("Error parsing message: %s", error->message);
      return;
    }

  root_object = json_node_get_object (json_parser_get_root (parser));
  uuid = json_object_get_string_member_with_default (root_object, "message-id", NULL);

  if (uuid)
    {
      GTask *task = g_hash_table_lookup (self->uuid_to_task, uuid);
      g_task_return_pointer (task, g_bytes_ref (message), NULL);
    }
  else
    {
      parse_event (self, root_object);
    }
}

static void
on_websocket_authenticated_cb (GObject      *source_object,
                               GAsyncResult *result,
                               gpointer      user_data)
{
  g_autoptr (JsonNode) node = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GTask) task = NULL;
  ObsConnection *self;
  JsonObject *object;

  node = parse_message_response (result, &error);
  object = json_node_get_object (node);

  if (error)
    {
      g_warning ("Error parsing message response: %s", error->message);
      return;
    }

  task = G_TASK (user_data);
  self = g_task_get_source_object (task);

  if (g_strcmp0 (json_object_get_string_member_with_default (object, "status", NULL), "ok") == 0)
    {
      save_password (self, g_task_get_task_data (task));
      g_task_return_boolean (task, TRUE);
      fetch_scenes (self);
    }
  else
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_PROXY_AUTH_FAILED, _("Invalid password"));
      set_connection_state (self, OBS_CONNECTION_STATE_WAITING_FOR_CREDENTIALS);
    }
}

static void
on_websocket_check_authentication_required_cb (GObject      *source_object,
                                               GAsyncResult *result,
                                               gpointer      user_data)
{
  g_autoptr (JsonNode) node = NULL;
  g_autoptr (GError) error = NULL;
  ObsConnection *self;
  JsonObject *object;

  node = parse_message_response (result, &error);
  object = json_node_get_object (node);

  if (error)
    {
      g_warning ("Error parsing message response: %s", error->message);
      return;
    }

  self = OBS_CONNECTION (user_data);

  if (json_object_get_boolean_member_with_default (object, "authRequired", FALSE))
    {
      g_autofree char *password = NULL;

      self->authentication.challenge = g_strdup (json_object_get_string_member_with_default (object,
                                                                                             "challenge",
                                                                                             NULL));
      self->authentication.salt = g_strdup (json_object_get_string_member_with_default (object,
                                                                                        "salt",
                                                                                        NULL));

      /* If there's a stored password, authenticate immediately */
      password = lookup_password (self);
      if (password)
        {
          /*
           * Set the state without emitting 'state-changed' so we can use
           * obs_connection_authenticate() directly.
           */
          self->state = OBS_CONNECTION_STATE_WAITING_FOR_CREDENTIALS;

          obs_connection_authenticate (self,
                                       password,
                                       self->cancellable,
                                       on_connection_authenticated_cb,
                                       self);
        }
      else
        {
          set_connection_state (self, OBS_CONNECTION_STATE_WAITING_FOR_CREDENTIALS);
        }
    }
  else
    {
      set_connection_state (self, OBS_CONNECTION_STATE_CONNECTED);
    }
}

static void
on_websocket_get_streaming_status_cb (GObject      *source_object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  g_autoptr (JsonNode) node = NULL;
  g_autoptr (GError) error = NULL;
  ObsConnection *self;
  JsonObject *object;
  gboolean recording_paused;
  gboolean recording;
  gboolean streaming;

  node = parse_message_response (result, &error);
  object = json_node_get_object (node);

  if (error)
    {
      g_warning ("Error parsing message response: %s", error->message);
      return;
    }

  self = OBS_CONNECTION (user_data);

  recording = json_object_get_boolean_member_with_default (object, "recording", FALSE);
  recording_paused = json_object_get_boolean_member_with_default (object, "recording-paused", FALSE);
  streaming = json_object_get_boolean_member_with_default (object, "streaming", FALSE);

  set_connection_state (self, OBS_CONNECTION_STATE_CONNECTED);

  set_streaming (self, streaming);
  if (!recording)
    set_recording_state (self, OBS_RECORDING_STATE_STOPPED);
  else if (recording_paused)
    set_recording_state (self, OBS_RECORDING_STATE_PAUSED);
  else
    set_recording_state (self, OBS_RECORDING_STATE_RECORDING);
}

static void
on_websocket_get_scene_list_cb (GObject      *source_object,
                                GAsyncResult *result,
                                gpointer      user_data)
{
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) node = NULL;
  g_autoptr (GError) error = NULL;
  ObsConnection *self;
  JsonObject *object;
  JsonNode *scenes_node;

  node = parse_message_response (result, &error);
  object = json_node_get_object (node);

  if (error)
    {
      g_warning ("Error parsing message response: %s", error->message);
      return;
    }

  self = OBS_CONNECTION (user_data);

  scenes_node = json_object_get_member (object, "scenes");
  if (JSON_NODE_HOLDS_ARRAY (scenes_node))
    {
      JsonArray *scenes_array;
      unsigned int i;

      scenes_array = json_node_get_array (scenes_node);
      for (i = 0; i < json_array_get_length (scenes_array); i++)
        {
          g_autoptr (ObsScene) scene = NULL;
          JsonObject *scene_object;

          scene_object = json_array_get_object_element (scenes_array, i);
          scene = obs_scene_new (json_object_get_string_member (scene_object, "name"));
          g_list_store_insert_sorted (self->scenes, scene, compare_scenes_by_name_cb, NULL);
        }
    }

  /* Fetch streaming state as well */
  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "request-type");
  json_builder_add_string_value (builder, "GetStreamingStatus");

  send_message (self, builder, self->cancellable, on_websocket_get_streaming_status_cb, self);
}

static void
on_websocket_recording_toggled_cb (GObject      *source_object,
                                   GAsyncResult *result,
                                   gpointer      user_data)
{
  g_autoptr (JsonNode) node = NULL;
  g_autoptr (GError) error = NULL;

  node = parse_message_response (result, &error);

  if (error)
    g_warning ("Error parsing message response: %s", error->message);
}

static void
on_websocket_scene_switched_cb (GObject      *source_object,
                                GAsyncResult *result,
                                gpointer      user_data)
{
  g_autoptr (JsonNode) node = NULL;
  g_autoptr (GError) error = NULL;

  node = parse_message_response (result, &error);

  if (error)
    g_warning ("Error parsing message response: %s", error->message);
}

static void
on_websocket_streaming_toggled_cb (GObject      *source_object,
                                   GAsyncResult *result,
                                   gpointer      user_data)
{
  g_autoptr (JsonNode) node = NULL;
  g_autoptr (GError) error = NULL;

  node = parse_message_response (result, &error);

  if (error)
    g_warning ("Error parsing message response: %s", error->message);
}

static void
websocket_connected_cb (GObject      *source_object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
	g_autoptr (SoupWebsocketConnection) websocket_client = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (GError) error = NULL;
  ObsConnection *self;

  websocket_client = soup_session_websocket_connect_finish (SOUP_SESSION (source_object),
                                                            result,
                                                            &error);

  if (error)
    {
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CONNECTION_REFUSED))
        g_warning ("Error connecting to websocket: %s", error->message);

      self = OBS_CONNECTION (user_data);
      set_connection_state (self, OBS_CONNECTION_STATE_DISCONNECTED);
      reconnect_after_timeout (self);
      return;
    }

  self = OBS_CONNECTION (user_data);
  self->websocket_client = g_steal_pointer (&websocket_client);
  g_signal_connect (self->websocket_client, "closed", G_CALLBACK (on_websocket_client_closed_cb), self);
  g_signal_connect (self->websocket_client, "error", G_CALLBACK (on_websocket_client_error_cb), self);
  g_signal_connect (self->websocket_client, "message", G_CALLBACK (on_websocket_client_message_cb), self);

  /* Check if the connection needs authentication */
  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "request-type");
  json_builder_add_string_value (builder, "GetAuthRequired");

  send_message (self, builder, self->cancellable, on_websocket_check_authentication_required_cb, self);
  set_connection_state (self, OBS_CONNECTION_STATE_AUTHENTICATING);
}


/*
 * GObject overrides
 */

static void
obs_connection_finalize (GObject *object)
{
  ObsConnection *self = (ObsConnection *)object;

  g_cancellable_cancel (self->cancellable);

  if (self->websocket_client)
    soup_websocket_connection_close (self->websocket_client, SOUP_WEBSOCKET_CLOSE_GOING_AWAY, NULL);

  g_clear_handle_id (&self->reconnect_timeout_id, g_source_remove);
  g_clear_pointer (&self->authentication.challenge, g_free);
  g_clear_pointer (&self->authentication.salt, g_free);
  g_clear_pointer (&self->host, g_free);
  g_clear_object (&self->websocket_client);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->session);
  g_clear_object (&self->scenes);

  G_OBJECT_CLASS (obs_connection_parent_class)->finalize (object);
}

static void
obs_connection_constructed (GObject *object)
{
  ObsConnection *self = (ObsConnection *)object;

  G_OBJECT_CLASS (obs_connection_parent_class)->constructed (object);

  connect_to_obs_websocket (self);
}

static void
obs_connection_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ObsConnection *self = OBS_CONNECTION (object);

  switch (prop_id)
    {
    case PROP_HOST:
      g_value_set_string (value, self->host);
      break;

    case PROP_PORT:
      g_value_set_uint (value, self->port);
      break;

    case PROP_RECORDING_STATE:
      g_value_set_int (value, self->recording_state);
      break;

    case PROP_STREAMING:
      g_value_set_boolean (value, self->streaming);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
obs_connection_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ObsConnection *self = OBS_CONNECTION (object);

  switch (prop_id)
    {
    case PROP_PORT:
      self->port = g_value_get_uint (value);
      break;

    case PROP_HOST:
      g_assert (self->host == NULL);
      self->host = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
obs_connection_class_init (ObsConnectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = obs_connection_finalize;
  object_class->constructed = obs_connection_constructed;
  object_class->get_property = obs_connection_get_property;
  object_class->set_property = obs_connection_set_property;

  properties[PROP_HOST] = g_param_spec_string ("host", NULL, NULL,
                                               NULL,
                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_PORT] = g_param_spec_uint ("port", NULL, NULL,
                                             0, G_MAXUINT, 0,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_RECORDING_STATE] = g_param_spec_int ("recording-state", NULL, NULL,
                                                       0, G_MAXINT, 0,
                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_STREAMING] = g_param_spec_boolean ("streaming", NULL, NULL,
                                                     FALSE,
                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals[STATE_CHANGED] = g_signal_new ("state-changed",
                                         OBS_TYPE_CONNECTION,
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL, NULL,
                                         G_TYPE_NONE,
                                         2,
                                         G_TYPE_INT,
                                         G_TYPE_INT);
}

static void
obs_connection_init (ObsConnection *self)
{
  self->session = soup_session_new ();
  self->cancellable = g_cancellable_new ();
  self->uuid_to_task = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  self->scenes = g_list_store_new (OBS_TYPE_SCENE);
  self->recording_state = OBS_RECORDING_STATE_STOPPED;
}

ObsConnection *
obs_connection_new (const char   *host,
                    unsigned int  port)
{
  return g_object_new (OBS_TYPE_CONNECTION,
                       "host", host,
                       "port", port,
                       NULL);
}

const char *
obs_connection_get_host (ObsConnection *self)
{
  g_return_val_if_fail (OBS_IS_CONNECTION (self), NULL);

  return self->host;
}

unsigned int
obs_connection_get_port (ObsConnection *self)
{
  g_return_val_if_fail (OBS_IS_CONNECTION (self), 0);

  return self->port;
}

ObsConnectionState
obs_connection_get_state (ObsConnection *self)
{
  g_return_val_if_fail (OBS_IS_CONNECTION (self), OBS_CONNECTION_STATE_DISCONNECTED);

  return self->state;
}

ObsRecordingState
obs_connection_get_recording_state (ObsConnection *self)
{
  g_return_val_if_fail (OBS_IS_CONNECTION (self), OBS_RECORDING_STATE_STOPPED);

  return self->recording_state;
}

gboolean
obs_connection_get_streaming (ObsConnection *self)
{
  g_return_val_if_fail (OBS_IS_CONNECTION (self), FALSE);

  return self->streaming;
}

void
obs_connection_authenticate (ObsConnection       *self,
                             const char          *password,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (GTask) task = NULL;
  g_autofree char *auth = NULL;

  g_return_if_fail (OBS_IS_CONNECTION (self));
  g_return_if_fail (password != NULL && g_utf8_validate (password, -1, NULL));

  if (self->state != OBS_CONNECTION_STATE_WAITING_FOR_CREDENTIALS)
    {
      g_warning ("Cannot authenticate when not waiting for credentials");
      return;
    }

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, g_strdup (password), g_free);
  g_task_set_source_tag (task, obs_connection_authenticate);

  set_connection_state (self, OBS_CONNECTION_STATE_AUTHENTICATING);

  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "request-type");
  json_builder_add_string_value (builder, "Authenticate");

  auth = generate_auth_string (password,
                               self->authentication.challenge,
                               self->authentication.salt);
  json_builder_set_member_name (builder, "auth");
  json_builder_add_string_value (builder, auth);
  send_message (self, builder, cancellable, on_websocket_authenticated_cb, g_steal_pointer (&task));
}

gboolean
obs_connection_authenticate_finish (ObsConnection  *self,
                                    GAsyncResult   *result,
                                    GError        **error)
{
  g_return_val_if_fail (OBS_IS_CONNECTION (self), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);
  g_return_val_if_fail (g_task_is_valid (G_TASK (result), self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == obs_connection_authenticate, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

GListModel *
obs_connection_get_scenes (ObsConnection *self)
{
  g_return_val_if_fail (OBS_IS_CONNECTION (self), NULL);

  return G_LIST_MODEL (self->scenes);
}

void
obs_connection_switch_to_scene (ObsConnection *self,
                                ObsScene      *scene)
{
  g_autoptr (JsonBuilder) builder = NULL;

  g_return_if_fail (OBS_IS_CONNECTION (self));
  g_return_if_fail (OBS_IS_SCENE (scene));
  g_return_if_fail (self->state == OBS_CONNECTION_STATE_CONNECTED);

  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "request-type");
  json_builder_add_string_value (builder, "SetCurrentScene");

  json_builder_set_member_name (builder, "scene-name");
  json_builder_add_string_value (builder, obs_scene_get_name (scene));

  send_message (self, builder, self->cancellable, on_websocket_scene_switched_cb, self);
}

void
obs_connection_toggle_recording (ObsConnection *self)
{
  g_autoptr (JsonBuilder) builder = NULL;

  g_return_if_fail (OBS_IS_CONNECTION (self));
  g_return_if_fail (self->state == OBS_CONNECTION_STATE_CONNECTED);

  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "request-type");
  json_builder_add_string_value (builder, "StartStopRecording");

  send_message (self, builder, self->cancellable, on_websocket_recording_toggled_cb, self);
}

void
obs_connection_toggle_streaming (ObsConnection *self)
{
  g_autoptr (JsonBuilder) builder = NULL;

  g_return_if_fail (OBS_IS_CONNECTION (self));
  g_return_if_fail (self->state == OBS_CONNECTION_STATE_CONNECTED);

  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "request-type");
  json_builder_add_string_value (builder, "StartStopStreaming");

  send_message (self, builder, self->cancellable, on_websocket_streaming_toggled_cb, self);
}