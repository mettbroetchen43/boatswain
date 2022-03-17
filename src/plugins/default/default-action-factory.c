/* default-action-factory.c
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
#include "default-action-factory.h"
#include "default-brightness-action.h"
#include "default-multi-action.h"
#include "default-switch-page-action.h"
#include "default-switch-profile-action.h"

#include <glib/gi18n.h>

struct _DefaultActionFactory
{
  PeasExtensionBase parent_instance;
};

static void bs_action_factory_iface_init (BsActionFactoryInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (DefaultActionFactory, default_action_factory, PEAS_TYPE_EXTENSION_BASE,
                               G_IMPLEMENT_INTERFACE (BS_TYPE_ACTION_FACTORY, bs_action_factory_iface_init));

static const BsActionInfo actions[] = {
  {
    .id = "default-switch-page-action",
    .icon_name = "folder-symbolic",
    .name = N_("Folder"),
    .description = NULL,
  },
  {
    .id = "default-switch-profile-action",
    .icon_name = "view-list-bullet-symbolic",
    .name = N_("Switch Profile"),
    .description = NULL,
  },
  {
    .id = "default-brightness-action",
    .icon_name = "display-brightness-symbolic",
    .name = N_("Brightness"),
    .description = NULL,
  },
  {
    .id = "default-multi-action",
    .icon_name = "stacked-plates-symbolic",
    .name = N_("Multiple Actions"),
    .description = NULL,
  },
};

static GList *
default_action_factory_list_actions (BsActionFactory *action_factory)
{
  GList *list = NULL;
  size_t i;

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    list = g_list_prepend (list, (gpointer) &actions[i]);

  return list;
}

static BsAction *
default_action_factory_create_action (BsActionFactory    *action_factory,
                                      BsStreamDeckButton *stream_deck_button,
                                      const BsActionInfo *action_info)
{
  if (g_strcmp0 (action_info->id, "default-switch-profile-action") == 0)
    return default_switch_profile_action_new (stream_deck_button);
  else if (g_strcmp0 (action_info->id, "default-brightness-action") == 0)
    return default_brightness_action_new (stream_deck_button);
  else if (g_strcmp0 (action_info->id, "default-switch-page-action") == 0)
    return default_switch_page_action_new (stream_deck_button);
  else if (g_strcmp0 (action_info->id, "default-multi-action") == 0)
    return default_multi_action_new (stream_deck_button);

  return NULL;
}

static void
bs_action_factory_iface_init (BsActionFactoryInterface *iface)
{
  iface->list_actions = default_action_factory_list_actions;
  iface->create_action = default_action_factory_create_action;
}

static void
default_action_factory_class_init (DefaultActionFactoryClass *klass)
{
}

static void
default_action_factory_init (DefaultActionFactory *self)
{
}
