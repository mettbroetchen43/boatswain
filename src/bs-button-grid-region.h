/*
 * bs-button-grid-region.h
 *
 * Copyright 2024 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#include "bs-device-region.h"
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BS_TYPE_BUTTON_GRID_REGION (bs_button_grid_region_get_type())
G_DECLARE_FINAL_TYPE (BsButtonGridRegion, bs_button_grid_region, BS, BUTTON_GRID_REGION, BsDeviceRegion)

BsButtonGridRegion * bs_button_grid_region_new (const char         *id,
                                                BsStreamDeck       *stream_deck,
                                                const BsIconLayout *icon_layout,
                                                unsigned int        n_buttons,
                                                unsigned int        grid_columns);

BsButtonGridRegion * bs_button_grid_region_new_full (const char         *id,
                                                     BsStreamDeck       *stream_deck,
                                                     const BsIconLayout *icon_layout,
                                                     unsigned int        n_buttons,
                                                     unsigned int        grid_columns,
                                                     unsigned int        column,
                                                     unsigned int        row,
                                                     unsigned int        column_span,
                                                     unsigned int        row_span);

GListModel * bs_button_grid_region_get_buttons (BsButtonGridRegion *self);

unsigned int bs_button_grid_region_get_grid_columns (BsButtonGridRegion *self);

G_END_DECLS
