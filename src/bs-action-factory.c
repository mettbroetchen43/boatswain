/* bs-action-factory.c
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
#include "bs-action-private.h"

G_DEFINE_INTERFACE (BsActionFactory, bs_action_factory, G_TYPE_OBJECT)

static void
bs_action_factory_default_init (BsActionFactoryInterface *iface)
{
}

GList *
bs_action_factory_list_actions (BsActionFactory *self)
{
  g_return_val_if_fail (BS_IS_ACTION_FACTORY (self), NULL);
  g_return_val_if_fail (BS_ACTION_FACTORY_GET_IFACE (self)->list_actions, NULL);

  return BS_ACTION_FACTORY_GET_IFACE (self)->list_actions (self);
}

const BsActionInfo *
bs_action_factory_get_info (BsActionFactory *self,
                            const char      *id)
{
  g_autoptr (GList) actions = NULL;
  GList *l;

  g_return_val_if_fail (BS_IS_ACTION_FACTORY (self), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  actions = bs_action_factory_list_actions (self);

  for (l = actions; l; l = l->next)
    {
      const BsActionInfo *info = l->data;

      if (g_strcmp0 (info->id, id) == 0)
        return info;
    }

  return NULL;
}

BsAction *
bs_action_factory_create_action (BsActionFactory    *self,
                                 BsStreamDeckButton *stream_deck_button,
                                 const BsActionInfo *action_info)
{
  BsAction *action;

  g_return_val_if_fail (BS_IS_ACTION_FACTORY (self), NULL);
  g_return_val_if_fail (BS_ACTION_FACTORY_GET_IFACE (self)->create_action, NULL);

  action = BS_ACTION_FACTORY_GET_IFACE (self)->create_action (self,
                                                              stream_deck_button,
                                                              action_info);

  if (!action)
    return NULL;

  bs_action_set_id (action, action_info->id);
  bs_action_set_name (action, action_info->name);
  bs_action_set_factory (action, self);

  return action;
}
