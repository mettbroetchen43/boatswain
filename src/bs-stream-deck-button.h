/* bs-stream-deck-button.h
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

#include "bs-types.h"

G_BEGIN_DECLS

#define BS_TYPE_STREAM_DECK_BUTTON (bs_stream_deck_button_get_type())
G_DECLARE_FINAL_TYPE (BsStreamDeckButton, bs_stream_deck_button, BS, STREAM_DECK_BUTTON, GObject)

BsStreamDeck * bs_stream_deck_button_get_stream_deck (BsStreamDeckButton *self);

uint8_t bs_stream_deck_button_get_position (BsStreamDeckButton *self);

gboolean bs_stream_deck_button_get_pressed (BsStreamDeckButton *self);

BsIcon * bs_stream_deck_button_get_icon (BsStreamDeckButton *self);

BsIcon * bs_stream_deck_button_get_custom_icon (BsStreamDeckButton *self);
void bs_stream_deck_button_set_custom_icon (BsStreamDeckButton *self,
                                            BsIcon             *icon);

BsAction * bs_stream_deck_button_get_action (BsStreamDeckButton *self);
void bs_stream_deck_button_set_action (BsStreamDeckButton *self,
                                       BsAction           *action);

G_END_DECLS
