/*
 * bs-touchscreen-region.h
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

#define BS_TYPE_TOUCHSCREEN_REGION (bs_touchscreen_region_get_type())
G_DECLARE_FINAL_TYPE (BsTouchscreenRegion, bs_touchscreen_region, BS, TOUCHSCREEN_REGION, BsDeviceRegion)

BsTouchscreenRegion * bs_touchscreen_region_new (const char   *id,
                                                 BsStreamDeck *stream_deck,
                                                 uint32_t      n_slots,
                                                 uint32_t      width,
                                                 uint32_t      height,
                                                 unsigned int  column,
                                                 unsigned int  row,
                                                 unsigned int  column_span,
                                                 unsigned int  row_span);

G_END_DECLS
