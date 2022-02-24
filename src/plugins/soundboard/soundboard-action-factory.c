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
#include "soundboard-play-action.h"

#include <glib/gi18n.h>

struct _SoundboardActionFactory
{
  PeasExtensionBase parent_instance;
};

static void bs_action_factory_iface_init (BsActionFactoryInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (SoundboardActionFactory, soundboard_action_factory, PEAS_TYPE_EXTENSION_BASE,
                               G_IMPLEMENT_INTERFACE (BS_TYPE_ACTION_FACTORY, bs_action_factory_iface_init));

static const BsActionInfo actions[] = {
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
                                         const BsActionInfo *action_info)
{
  if (g_strcmp0 (action_info->id, "soundboard-play-action") == 0)
    return soundboard_play_action_new ();

  return NULL;
}

static void
bs_action_factory_iface_init (BsActionFactoryInterface *iface)
{
  iface->list_actions = soundboard_action_factory_list_actions;
  iface->create_action = soundboard_action_factory_create_action;
}

static void
soundboard_action_factory_class_init (SoundboardActionFactoryClass *klass)
{
}

static void
soundboard_action_factory_init (SoundboardActionFactory *self)
{
}
