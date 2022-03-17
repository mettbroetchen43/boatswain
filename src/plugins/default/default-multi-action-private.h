/* default-multi-action-private.h
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

#include "default-multi-action.h"

G_BEGIN_DECLS

typedef enum
{
  MULTI_ACTION_ENTRY_ACTION,
  MULTI_ACTION_ENTRY_DELAY,
} MultiActionEntryType;

typedef struct
{
  MultiActionEntryType entry_type;
  union {
    unsigned int delay_ms;
    BsAction *action;
  } v;
} MultiActionEntry;

GPtrArray * default_multi_action_get_entries (DefaultMultiAction *self);

void default_multi_action_add_action (DefaultMultiAction *self,
                                      BsAction           *action);

void default_multi_action_add_delay (DefaultMultiAction *self,
                                     unsigned int        delay_ms);

G_END_DECLS
