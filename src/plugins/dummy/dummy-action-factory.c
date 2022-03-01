/* dummy-action-factory.c
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
#include "dummy-action-factory.h"
#include "dummy-action-1.h"

struct _DummyActionFactory
{
  PeasExtensionBase parent_instance;
};

static void bs_action_factory_iface_init (BsActionFactoryInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (DummyActionFactory, dummy_action_factory, PEAS_TYPE_EXTENSION_BASE,
                               G_IMPLEMENT_INTERFACE (BS_TYPE_ACTION_FACTORY, bs_action_factory_iface_init));

static const BsActionInfo actions[] = {
  { "dummy-action1", "action-unavailable-symbolic", "Dummy action 1", "This is a dummy action" },
};

static GList *
dummy_action_factory_list_actions (BsActionFactory *action_factory)
{
  GList *list = NULL;
  size_t i;

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    list = g_list_prepend (list, (gpointer) &actions[i]);

  return list;
}

static BsAction *
dummy_action_factory_create_action (BsActionFactory    *action_factory,
                                    BsStreamDeckButton *stream_deck_button,
                                    const BsActionInfo *action_info)
{
  size_t i;

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
      if (action_info == &actions[i])
        return g_object_new (DUMMY_TYPE_ACTION_1, "stream-deck-button", stream_deck_button, NULL);
    }

  return NULL;
}

static void
bs_action_factory_iface_init (BsActionFactoryInterface *iface)
{
  iface->list_actions = dummy_action_factory_list_actions;
  iface->create_action = dummy_action_factory_create_action;
}

static void
dummy_action_factory_class_init (DummyActionFactoryClass *klass)
{
}

static void
dummy_action_factory_init (DummyActionFactory *self)
{
}
