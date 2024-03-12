/*
 * bs-touchscreen.c
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

#include "bs-touchscreen.h"

struct _BsTouchscreen
{
  GObject parent_instance;

  uint32_t width;
  uint32_t height;

  BsStreamDeck *stream_deck;
};

G_DEFINE_FINAL_TYPE (BsTouchscreen, bs_touchscreen, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

static void
bs_touchscreen_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BsTouchscreen *self = BS_TOUCHSCREEN (object);

  switch (prop_id)
    {
    case PROP_WIDTH:
      g_value_set_uint (value, self->width);
      break;

    case PROP_HEIGHT:
      g_value_set_uint (value, self->height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_touchscreen_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BsTouchscreen *self = BS_TOUCHSCREEN (object);

  switch (prop_id)
    {
    case PROP_WIDTH:
      self->width = g_value_get_uint (value);
      break;

    case PROP_HEIGHT:
      self->height = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_touchscreen_class_init (BsTouchscreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bs_touchscreen_get_property;
  object_class->set_property = bs_touchscreen_set_property;

  properties[PROP_WIDTH] = g_param_spec_uint ("width", NULL, NULL,
                                              1, G_MAXUINT, 1,
                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_HEIGHT] = g_param_spec_uint ("height", NULL, NULL,
                                               1, G_MAXUINT, 1,
                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_touchscreen_init (BsTouchscreen *self)
{
  self->width = 1;
  self->height = 1;
}

BsTouchscreen *
bs_touchscreen_new (BsStreamDeck *stream_deck,
                    uint32_t      width,
                    uint32_t      height)
{
  g_autoptr (BsTouchscreen) self = NULL;

  self = g_object_new (BS_TYPE_TOUCHSCREEN,
                       "width", width,
                       "height", height,
                       NULL);
  self->stream_deck = stream_deck;

  return g_steal_pointer (&self);
}

BsStreamDeck *
bs_touchscreen_get_stream_deck (BsTouchscreen *self)
{
  g_assert (BS_IS_TOUCHSCREEN (self));

  return self->stream_deck;
}

uint32_t
bs_dial_get_width (BsTouchscreen *self)
{
  g_assert (BS_IS_TOUCHSCREEN (self));

  return self->width;
}

uint32_t
bs_dial_get_height (BsTouchscreen *self)
{
  g_assert (BS_IS_TOUCHSCREEN (self));

  return self->height;
}
