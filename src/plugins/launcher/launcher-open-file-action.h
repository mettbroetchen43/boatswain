/*
 * launcher-open-file-action.h
 *
 * Copyright 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#include "bs-action.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_OPEN_FILE_ACTION (launcher_open_file_action_get_type())
G_DECLARE_FINAL_TYPE (LauncherOpenFileAction,
                      launcher_open_file_action,
                      LAUNCHER, OPEN_FILE_ACTION,
                      BsAction)

BsAction * launcher_open_file_action_new (BsStreamDeckButton *stream_deck_button);

G_END_DECLS
