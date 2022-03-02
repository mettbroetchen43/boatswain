/* default-switch-page-action.h
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

#include "bs-action.h"
#include "bs-types.h"

G_BEGIN_DECLS

#define DEFAULT_TYPE_SWITCH_PAGE_ACTION (default_switch_page_action_get_type())
G_DECLARE_FINAL_TYPE (DefaultSwitchPageAction, default_switch_page_action, DEFAULT, SWITCH_PAGE_ACTION, BsAction)

BsAction * default_switch_page_action_new (BsStreamDeckButton *stream_deck_button);

G_END_DECLS
