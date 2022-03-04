/* obs-connection.h
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

#pragma once

#include <gio/gio.h>

#include "obs-scene.h"

G_BEGIN_DECLS

typedef enum
{
  OBS_CONNECTION_STATE_DISCONNECTED,
  OBS_CONNECTION_STATE_CONNECTING,
  OBS_CONNECTION_STATE_AUTHENTICATING,
  OBS_CONNECTION_STATE_WAITING_FOR_CREDENTIALS,
  OBS_CONNECTION_STATE_CONNECTED,
} ObsConnectionState;

typedef enum
{
  OBS_RECORDING_STATE_STOPPED,
  OBS_RECORDING_STATE_PAUSED,
  OBS_RECORDING_STATE_RECORDING,
} ObsRecordingState;

typedef enum
{
  OBS_STREAMING_STATE_STOPPED,
  OBS_STREAMING_STATE_STREAMING,
} ObsStreamingState;

#define OBS_TYPE_CONNECTION (obs_connection_get_type())
G_DECLARE_FINAL_TYPE (ObsConnection, obs_connection, OBS, CONNECTION, GObject)

ObsConnection * obs_connection_new (const char   *host,
                                    unsigned int  port);

const char * obs_connection_get_host (ObsConnection *self);

unsigned int obs_connection_get_port (ObsConnection *self);

ObsConnectionState obs_connection_get_state (ObsConnection *self);
ObsRecordingState obs_connection_get_recording_state (ObsConnection *self);
gboolean obs_connection_get_streaming (ObsConnection *self);

void obs_connection_authenticate (ObsConnection       *self,
                                  const char          *password,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data);

gboolean obs_connection_authenticate_finish (ObsConnection  *self,
                                             GAsyncResult   *result,
                                             GError        **error);

GListModel * obs_connection_get_scenes (ObsConnection *self);

void obs_connection_switch_to_scene (ObsConnection *self,
                                     ObsScene      *scene);

void obs_connection_toggle_recording (ObsConnection *self);
void obs_connection_toggle_streaming (ObsConnection *self);

G_END_DECLS
