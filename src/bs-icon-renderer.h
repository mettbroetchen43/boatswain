/* bs-icon-renderer.h
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
#include <stdint.h>

#include "bs-types.h"

G_BEGIN_DECLS

enum _BsIconRendererFlags
{
  BS_ICON_RENDERER_FLAG_NONE = 0,
  BS_ICON_RENDERER_FLAG_FLIP_X = 1 << 0,
  BS_ICON_RENDERER_FLAG_FLIP_Y = 1 << 1,
  BS_ICON_RENDERER_FLAG_ROTATE_90 = 1 << 2,
};

enum _BsIconFormat
{
  BS_ICON_FORMAT_BMP,
  BS_ICON_FORMAT_JPEG,
};

enum _BsIconComposeFlags
{
  BS_ICON_COMPOSE_FLAG_NONE = 0,
  BS_ICON_COMPOSE_FLAG_IGNORE_TRANSFORMS = 1 << 0,
};

struct _BsIconLayout
{
  BsIconFormat format;
  BsIconRendererFlags flags;
  int8_t width;
  int8_t height;
};

#define BS_TYPE_ICON_RENDERER (bs_icon_renderer_get_type())
G_DECLARE_FINAL_TYPE (BsIconRenderer, bs_icon_renderer, BS, ICON_RENDERER, GObject)

BsIconRenderer * bs_icon_renderer_new (const BsIconLayout *layout);

GdkTexture * bs_icon_renderer_compose_icon (BsIconRenderer      *self,
                                            BsIconComposeFlags   compose_flags,
                                            BsIcon              *icon,
                                            GError             **error);

gboolean bs_icon_renderer_convert_texture (BsIconRenderer  *self,
                                           GdkTexture      *texture,
                                           char           **buffer,
                                           size_t          *buffer_len,
                                           GError         **error);

G_END_DECLS
