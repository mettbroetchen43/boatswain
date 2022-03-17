/* default-multi-action-row.h
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

#include <adwaita.h>

#include "default-multi-action-private.h"

G_BEGIN_DECLS

#define DEFAULT_TYPE_MULTI_ACTION_ROW (default_multi_action_row_get_type())
G_DECLARE_FINAL_TYPE (DefaultMultiActionRow, default_multi_action_row, DEFAULT, MULTI_ACTION_ROW, AdwActionRow)

GtkWidget * default_multi_action_row_new (MultiActionEntry *entry);

MultiActionEntry * default_multi_action_row_get_entry (DefaultMultiActionRow *self);

G_END_DECLS
