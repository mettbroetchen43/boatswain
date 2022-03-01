/* bs-action-factory.h
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

#include <glib-object.h>

#include "bs-types.h"

G_BEGIN_DECLS

#define BS_TYPE_ACTION_FACTORY (bs_action_factory_get_type ())
G_DECLARE_INTERFACE (BsActionFactory, bs_action_factory, BS, ACTION_FACTORY, GObject)

typedef struct
{
  const char *id;
  const char *icon_name;
  const char *name;
  const char *description;
} BsActionInfo;

struct _BsActionFactoryInterface
{
  GTypeInterface parent;

  GList * (*list_actions) (BsActionFactory *self);
  BsAction * (*create_action) (BsActionFactory    *self,
                               BsStreamDeckButton *stream_deck_button,
                               const BsActionInfo *action_info);
};

GList * bs_action_factory_list_actions (BsActionFactory *self);

const BsActionInfo * bs_action_factory_get_info (BsActionFactory *self,
                                                 const char      *id);

BsAction * bs_action_factory_create_action (BsActionFactory    *self,
                                            BsStreamDeckButton *stream_deck_button,
                                            const BsActionInfo *action_info);

G_END_DECLS
