/*
 * bs-stream-deck-button-widget.h
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

#include "bs-types.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BS_TYPE_STREAM_DECK_BUTTON_WIDGET (bs_stream_deck_button_widget_get_type())
G_DECLARE_FINAL_TYPE (BsStreamDeckButtonWidget, bs_stream_deck_button_widget, BS, STREAM_DECK_BUTTON_WIDGET, GtkFlowBoxChild)

GtkWidget *bs_stream_deck_button_widget_new (BsStreamDeckButton *button);

BsStreamDeckButton * bs_stream_deck_button_widget_get_button (BsStreamDeckButtonWidget *self);

G_END_DECLS
