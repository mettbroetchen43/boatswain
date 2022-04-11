/* soundboard-action-factory.c
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
#include "soundboard-action-factory.h"
#include "soundboard-mpris-action.h"
#include "soundboard-play-action.h"

#include <glib/gi18n.h>

struct _SoundboardActionFactory
{
  PeasExtensionBase parent_instance;

  MprisController *mpris_controller;
};

static void bs_action_factory_iface_init (BsActionFactoryInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (SoundboardActionFactory, soundboard_action_factory, PEAS_TYPE_EXTENSION_BASE,
                               G_IMPLEMENT_INTERFACE (BS_TYPE_ACTION_FACTORY, bs_action_factory_iface_init));

static const BsActionInfo actions[] = {
  {
    .id = "soundboard-mpris-action",
    .icon_name = "audio-x-generic-symbolic",
    .name = N_("Music Player"),
    .description = N_("Control the active music player"),
  },
  {
    .id = "soundboard-play-action",
    .icon_name = "media-playback-start-symbolic",
    .name = N_("Play Audio"),
    .description = NULL,
  },
};

static GList *
soundboard_action_factory_list_actions (BsActionFactory *action_factory)
{
  GList *list = NULL;
  size_t i;

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    list = g_list_prepend (list, (gpointer) &actions[i]);

  return list;
}

static BsAction *
soundboard_action_factory_create_action (BsActionFactory    *action_factory,
                                         BsStreamDeckButton *stream_deck_button,
                                         const BsActionInfo *action_info)
{
  SoundboardActionFactory *self = SOUNDBOARD_ACTION_FACTORY (action_factory);

  if (g_strcmp0 (action_info->id, "soundboard-play-action") == 0)
    return soundboard_play_action_new (stream_deck_button);
  else if (g_strcmp0 (action_info->id, "soundboard-mpris-action") == 0)
    return soundboard_mpris_action_new (stream_deck_button, self->mpris_controller);

  return NULL;
}

static void
bs_action_factory_iface_init (BsActionFactoryInterface *iface)
{
  iface->list_actions = soundboard_action_factory_list_actions;
  iface->create_action = soundboard_action_factory_create_action;
}

static void
soundboard_action_factory_finalize (GObject *object)
{
  SoundboardActionFactory *self = SOUNDBOARD_ACTION_FACTORY (object);

  g_clear_object (&self->mpris_controller);

  G_OBJECT_CLASS (soundboard_action_factory_parent_class)->finalize (object);
}

static void
soundboard_action_factory_class_init (SoundboardActionFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = soundboard_action_factory_finalize;
}

static void
soundboard_action_factory_init (SoundboardActionFactory *self)
{
  self->mpris_controller = mpris_controller_new ();
}
