/* bs-button-editor.h
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

#include "bs-types.h"

G_BEGIN_DECLS

#define BS_TYPE_BUTTON_EDITOR (bs_button_editor_get_type())
G_DECLARE_FINAL_TYPE (BsButtonEditor, bs_button_editor, BS, BUTTON_EDITOR, AdwBin)

GtkWidget * bs_button_editor_new (BsButton *button);

G_END_DECLS
