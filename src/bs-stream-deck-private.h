/* bs-stream-deck-private.h
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

#include "bs-stream-deck.h"

#include <gusb.h>

G_BEGIN_DECLS

typedef struct
{
  uint8_t n_buttons;
  uint8_t rows;
  uint8_t columns;
  uint32_t icon_size;
} BsStreamDeckButtonLayout;

BsStreamDeck * bs_stream_deck_new (GUsbDevice  *gusb_device,
                                   GError     **error);

BsStreamDeck * bs_stream_deck_new_fake (GError **error);

GUsbDevice * bs_stream_deck_get_device (BsStreamDeck *self);

const BsStreamDeckButtonLayout * bs_stream_deck_get_button_layout (BsStreamDeck *self);

BsStreamDeckButton * bs_stream_deck_get_button (BsStreamDeck *self,
                                                uint8_t       position);

gboolean bs_stream_deck_upload_button (BsStreamDeck        *self,
                                       BsStreamDeckButton  *button,
                                       GError             **error);

void bs_stream_deck_load (BsStreamDeck *self);

void bs_stream_deck_save (BsStreamDeck *self);

G_END_DECLS
