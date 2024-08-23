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

#include "bs-renderer.h"
#include "bs-stream-deck.h"
#include "bs-touchscreen-private.h"

struct _BsTouchscreenRegion
{
  BsDeviceRegion parent_instance;

  BsTouchscreen *touchscreen;
  BsRenderer *renderer;
};

G_DEFINE_FINAL_TYPE (BsTouchscreenRegion, bs_touchscreen_region, BS_TYPE_DEVICE_REGION)

enum {
  PROP_0,
  PROP_TOUCHSCREEN,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];


/*
 * BsDeviceRegion overrides
 */

static BsRenderer *
bs_touchscreen_region_get_renderer (BsDeviceRegion *region)
{
  BsTouchscreenRegion *self = (BsTouchscreenRegion *) region;

  g_assert (BS_IS_TOUCHSCREEN_REGION (self));

  return self->renderer;
}


/*
 * GObject overrides
 */

static void
bs_touchscreen_region_finalize (GObject *object)
{
  BsTouchscreenRegion *self = (BsTouchscreenRegion *)object;

  g_clear_object (&self->touchscreen);
  g_clear_object (&self->renderer);

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
  BsDeviceRegionClass *device_region_class = BS_DEVICE_REGION_CLASS (klass);

  object_class->finalize = bs_touchscreen_region_finalize;
  object_class->get_property = bs_touchscreen_region_get_property;
  object_class->set_property = bs_touchscreen_region_set_property;

  device_region_class->get_renderer = bs_touchscreen_region_get_renderer;

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
bs_touchscreen_region_new (const char        *id,
                           BsStreamDeck      *stream_deck,
                           const BsImageInfo *image_info,
                           uint32_t           n_slots,
                           unsigned int       column,
                           unsigned int       row,
                           unsigned int       column_span,
                           unsigned int       row_span)
{
  g_autoptr (BsTouchscreenRegion) self = NULL;

  g_assert (BS_IS_STREAM_DECK (stream_deck));
  g_assert (image_info != NULL);

  self = g_object_new (BS_TYPE_TOUCHSCREEN_REGION,
                       "id", id,
                       "stream-deck", stream_deck,
                       "column", column,
                       "row", row,
                       "column-span", column_span,
                       "row-span", row_span,
                       NULL);

  self->renderer = bs_renderer_new (image_info);
  self->touchscreen = bs_touchscreen_new (BS_DEVICE_REGION (self),
                                          n_slots,
                                          image_info->width,
                                          image_info->height);

  return g_steal_pointer (&self);
}
