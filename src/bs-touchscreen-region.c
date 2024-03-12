/*
 * bs-touchscreen-region.c
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

#include "bs-touchscreen-region.h"

#include "bs-stream-deck.h"
#include "bs-touchscreen-private.h"

struct _BsTouchscreenRegion
{
  BsDeviceRegion parent_instance;

  BsTouchscreen *touchscreen;
};

G_DEFINE_FINAL_TYPE (BsTouchscreenRegion, bs_touchscreen_region, BS_TYPE_DEVICE_REGION)

enum {
  PROP_0,
  PROP_TOUCHSCREEN,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

static void
bs_touchscreen_region_finalize (GObject *object)
{
  BsTouchscreenRegion *self = (BsTouchscreenRegion *)object;

  g_clear_object (&self->touchscreen);

  G_OBJECT_CLASS (bs_touchscreen_region_parent_class)->finalize (object);
}

static void
bs_touchscreen_region_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BsTouchscreenRegion *self = BS_TOUCHSCREEN_REGION (object);

  switch (prop_id)
    {
    case PROP_TOUCHSCREEN:
      g_value_set_object (value, self->touchscreen);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_touchscreen_region_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  //BsTouchscreenRegion *self = BS_TOUCHSCREEN_REGION (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_touchscreen_region_class_init (BsTouchscreenRegionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_touchscreen_region_finalize;
  object_class->get_property = bs_touchscreen_region_get_property;
  object_class->set_property = bs_touchscreen_region_set_property;

  properties[PROP_TOUCHSCREEN] = g_param_spec_object ("touchscreen", NULL, NULL,
                                                      BS_TYPE_TOUCHSCREEN,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_touchscreen_region_init (BsTouchscreenRegion *self)
{
}

BsTouchscreenRegion *
bs_touchscreen_region_new (const char   *id,
                           BsStreamDeck *stream_deck,
                           uint32_t      width,
                           uint32_t      height,
                           unsigned int  column,
                           unsigned int  row,
                           unsigned int  column_span,
                           unsigned int  row_span)
{
  g_autoptr (BsTouchscreenRegion) self = NULL;

  g_assert (BS_IS_STREAM_DECK (stream_deck));
  g_assert (width > 0);
  g_assert (height > 0);

  self = g_object_new (BS_TYPE_TOUCHSCREEN_REGION,
                       "id", id,
                       "stream-deck", stream_deck,
                       "column", column,
                       "row", row,
                       "column-span", column_span,
                       "row-span", row_span,
                       NULL);

  self->touchscreen = bs_touchscreen_new (stream_deck,  width, height);

  return g_steal_pointer (&self);
}
