/* bs-device-enums.h
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

#include <glib.h>

G_BEGIN_DECLS

#define ELGATO_SYSTEMS_VENDOR_ID (0x0fd9)

#define STREAMDECK_ORIGINAL_PRODUCT_ID 0x0060
#define STREAMDECK_ORIGINAL_V2_PRODUCT_ID  0x006d
#define STREAMDECK_MINI_PRODUCT_ID  0x0063
#define STREAMDECK_MINI_V2_PRODUCT_ID  0x0090
#define STREAMDECK_XL_PRODUCT_ID  0x006c
#define STREAMDECK_XL_V2_PRODUCT_ID  0x008f
#define STREAMDECK_MK2_PRODUCT_ID  0x0080
#define STREAMDECK_PEDAL_PRODUCT_ID  0x0086
#define STREAMDECK_PLUS_PRODUCT_ID  0x0084
#define STREAMDECK_NEO_PRODUCT_ID  0x009a

G_END_DECLS
