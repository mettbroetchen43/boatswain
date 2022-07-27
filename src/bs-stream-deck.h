/* bs-stream-deck.h
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
#include <gusb.h>
#include <stdint.h>

#include "bs-types.h"

G_BEGIN_DECLS

/**
 * BsStreamDeckError:
 * @BS_STREAM_DECK_ERROR_UNRECOGNIZED: not a recognized Stream Deck device
 *
 * Errors that #BsStreamDeck can generate.
 */
typedef enum
{
  BS_STREAM_DECK_ERROR_UNRECOGNIZED,
} BsStreamDeckError;

#define BS_STREAM_DECK_ERROR (bs_stream_deck_error_quark ())
GQuark  bs_stream_deck_error_quark (void);

typedef struct
{
  uint8_t n_buttons;
  uint8_t rows;
  uint8_t columns;
  uint32_t icon_size;
} BsStreamDeckButtonLayout;

#define BS_TYPE_STREAM_DECK (bs_stream_deck_get_type())
G_DECLARE_FINAL_TYPE (BsStreamDeck, bs_stream_deck, BS, STREAM_DECK, GObject)

BsStreamDeck * bs_stream_deck_new (GUsbDevice  *gusb_device,
                                   GError     **error);

BsStreamDeck * bs_stream_deck_new_fake (GError **error);

void bs_stream_deck_reset (BsStreamDeck *self);

GUsbDevice * bs_stream_deck_get_device (BsStreamDeck *self);

const char * bs_stream_deck_get_name (BsStreamDeck *self);

const char * bs_stream_deck_get_serial_number (BsStreamDeck *self);

const char * bs_stream_deck_get_firmware_version (BsStreamDeck *self);

GIcon * bs_stream_deck_get_icon (BsStreamDeck *self);

const BsStreamDeckButtonLayout * bs_stream_deck_get_button_layout (BsStreamDeck *self);

double bs_stream_deck_get_brightness (BsStreamDeck *self);
void bs_stream_deck_set_brightness (BsStreamDeck *self,
                                    double        brightness);

BsIconRenderer * bs_stream_deck_get_icon_renderer (BsStreamDeck *self);

BsStreamDeckButton * bs_stream_deck_get_button (BsStreamDeck *self,
                                                uint8_t       position);

GListModel * bs_stream_deck_get_profiles (BsStreamDeck *self);

BsProfile * bs_stream_deck_get_active_profile (BsStreamDeck *self);

void bs_stream_deck_load_profile (BsStreamDeck *self,
                                  BsProfile    *profile);

BsPage * bs_stream_deck_get_active_page (BsStreamDeck *self);

void bs_stream_deck_push_page (BsStreamDeck  *self,
                               BsPage        *page);

void bs_stream_deck_pop_page (BsStreamDeck *self);

G_END_DECLS
