/* default-multi-action-editor.h
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

#include <gtk/gtk.h>

#include "default-multi-action.h"

G_BEGIN_DECLS

#define DEFAULT_TYPE_MULTI_ACTION_EDITOR (default_multi_action_editor_get_type())
G_DECLARE_FINAL_TYPE (DefaultMultiActionEditor, default_multi_action_editor, DEFAULT, MULTI_ACTION_EDITOR, GtkBox)

GtkWidget * default_multi_action_editor_new (DefaultMultiAction *multi_action);

void default_multi_action_editor_update_entries (DefaultMultiActionEditor *self);

G_END_DECLS
