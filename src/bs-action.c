/* bs-action.c
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

#include "bs-action.h"

G_DEFINE_INTERFACE (BsAction, bs_action, G_TYPE_OBJECT)

static void
bs_action_default_init (BsActionInterface *iface)
{
}

BsIcon *
bs_action_get_icon (BsAction *self)
{
  g_return_val_if_fail (BS_IS_ACTION (self), NULL);
  g_return_val_if_fail (BS_ACTION_GET_IFACE (self)->get_icon, NULL);

  return BS_ACTION_GET_IFACE (self)->get_icon (self);
}

void
bs_action_activate (BsAction *self)
{
  g_return_if_fail (BS_IS_ACTION (self));

  if (BS_ACTION_GET_IFACE (self)->activate)
    BS_ACTION_GET_IFACE (self)->activate (self);
}

void
bs_action_deactivate (BsAction *self)
{
  g_return_if_fail (BS_IS_ACTION (self));

  if (BS_ACTION_GET_IFACE (self)->deactivate)
    BS_ACTION_GET_IFACE (self)->deactivate (self);
}
