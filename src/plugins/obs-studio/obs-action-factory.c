/* obs-action-factory.c
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

#include "bs-action-factory.h"
#include "obs-action-factory.h"
#include "obs-connection-manager.h"
#include "obs-record-action.h"
#include "obs-stream-action.h"
#include "obs-switch-scene-action.h"
#include "obs-toggle-source-action.h"
#include "obs-virtualcam-action.h"

#include <glib/gi18n.h>

struct _ObsActionFactory
{
  PeasExtensionBase parent_instance;

  ObsConnectionManager *connection_manager;
};

static void bs_action_factory_iface_init (BsActionFactoryInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ObsActionFactory, obs_action_factory, PEAS_TYPE_EXTENSION_BASE,
                               G_IMPLEMENT_INTERFACE (BS_TYPE_ACTION_FACTORY, bs_action_factory_iface_init));

static const BsActionInfo actions[] = {
  {
    .id = "obs-switch-scene-action",
    .icon_name = "obs-switch-scene-symbolic",
    .name = N_("Switch Scene"),
    .description = NULL,
  },
  {
    .id = "obs-record-action",
    .icon_name = "media-record-symbolic",
    /* Translators: "Record" is a verb here */
    .name = N_("Record"),
    .description = NULL,
  },
  {
    .id = "obs-stream-action",
    .icon_name = "transmit-symbolic",
    /* Translators: "Stream" is a verb here */
    .name = N_("Stream"),
    .description = NULL,
  },
  {
    .id = "obs-virtualcam-action",
    .icon_name = "cameras-symbolic",
    .name = N_("Virtual Camera"),
    .description = NULL,
  },
  {
    .id = "obs-toggle-mute-action",
    .icon_name = "audio-volume-muted-symbolic",
    .name = N_("Toggle Mute"),
    .description = NULL,
  },
  {
    .id = "obs-toggle-source-action",
    .icon_name = "eye-not-looking-symbolic",
    .name = N_("Show / Hide Source"),
    .description = NULL,
  },
};

static GList *
obs_action_factory_list_actions (BsActionFactory *action_factory)
{
  GList *list = NULL;
  size_t i;

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    list = g_list_prepend (list, (gpointer) &actions[i]);

  return g_list_reverse (list);
}

static BsAction *
obs_action_factory_create_action (BsActionFactory    *action_factory,
                                  BsStreamDeckButton *stream_deck_button,
                                  const BsActionInfo *action_info)
{
  ObsActionFactory *self = OBS_ACTION_FACTORY (action_factory);

  if (g_strcmp0 (action_info->id, "obs-switch-scene-action") == 0)
    return obs_switch_scene_action_new (stream_deck_button, self->connection_manager);
  else if (g_strcmp0 (action_info->id, "obs-record-action") == 0)
    return obs_record_action_new (stream_deck_button, self->connection_manager);
  else if (g_strcmp0 (action_info->id, "obs-stream-action") == 0)
    return obs_stream_action_new (stream_deck_button, self->connection_manager);
  else if (g_strcmp0 (action_info->id, "obs-virtualcam-action") == 0)
    return obs_virtualcam_action_new (stream_deck_button, self->connection_manager);
  else if (g_strcmp0 (action_info->id, "obs-toggle-mute-action") == 0)
    return obs_toggle_source_action_new (stream_deck_button, self->connection_manager, OBS_SOURCE_CAP_AUDIO);
  else if (g_strcmp0 (action_info->id, "obs-toggle-source-action") == 0)
    return obs_toggle_source_action_new (stream_deck_button, self->connection_manager, OBS_SOURCE_CAP_VIDEO);

  return NULL;
}

static void
bs_action_factory_iface_init (BsActionFactoryInterface *iface)
{
  iface->list_actions = obs_action_factory_list_actions;
  iface->create_action = obs_action_factory_create_action;
}

static void
obs_action_factory_finalize (GObject *object)
{
  ObsActionFactory *self = OBS_ACTION_FACTORY (object);

  g_clear_object (&self->connection_manager);

  G_OBJECT_CLASS (obs_action_factory_parent_class)->finalize (object);
}

static void
obs_action_factory_class_init (ObsActionFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = obs_action_factory_finalize;
}

static void
obs_action_factory_init (ObsActionFactory *self)
{
  self->connection_manager = obs_connection_manager_new ();
}
