/*
 * bs-device-region.h
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

#include <glib-object.h>

G_BEGIN_DECLS

#define BS_TYPE_DEVICE_REGION (bs_device_region_get_type())
G_DECLARE_DERIVABLE_TYPE (BsDeviceRegion, bs_device_region, BS, DEVICE_REGION, GObject)

struct _BsDeviceRegionClass
{
  GObjectClass parent_class;

  BsRenderer * (*get_renderer) (BsDeviceRegion *self);
};

const char * bs_device_region_get_id (BsDeviceRegion *self);

unsigned int bs_device_region_get_column (BsDeviceRegion *self);
unsigned int bs_device_region_get_column_span (BsDeviceRegion *self);

unsigned int bs_device_region_get_row (BsDeviceRegion *self);
unsigned int bs_device_region_get_row_span (BsDeviceRegion *self);

BsStreamDeck * bs_device_region_get_stream_deck (BsDeviceRegion *self);

BsRenderer * bs_device_region_get_renderer (BsDeviceRegion *self);

G_END_DECLS
