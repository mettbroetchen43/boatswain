/*
 * bs-touchscreen-slot.c
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

#include "bs-touchscreen-slot.h"

#include "bs-touchscreen.h"

#include <graphene.h>

struct _BsTouchscreenSlot
{
  GObject parent_instance;

  graphene_size_t size;

  BsTouchscreen *touchscreen;
};

G_DEFINE_FINAL_TYPE (BsTouchscreenSlot, bs_touchscreen_slot, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_TOUCHSCREEN,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];


/*
 * GObject overrides
 */

static void
bs_touchscreen_slot_finalize (GObject *object)
{
  //BsTouchscreenSlot *self = (BsTouchscreenSlot *)object;

  G_OBJECT_CLASS (bs_touchscreen_slot_parent_class)->finalize (object);
}

static void
bs_touchscreen_slot_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BsTouchscreenSlot *self = BS_TOUCHSCREEN_SLOT (object);

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
bs_touchscreen_slot_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BsTouchscreenSlot *self = BS_TOUCHSCREEN_SLOT (object);

  switch (prop_id)
    {
    case PROP_TOUCHSCREEN:
      g_assert (self->touchscreen == NULL);
      self->touchscreen = g_value_get_object (value);
      g_assert (BS_IS_TOUCHSCREEN (self->touchscreen));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_touchscreen_slot_class_init (BsTouchscreenSlotClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_touchscreen_slot_finalize;
  object_class->get_property = bs_touchscreen_slot_get_property;
  object_class->set_property = bs_touchscreen_slot_set_property;

  properties[PROP_TOUCHSCREEN] = g_param_spec_object ("touchscreen", NULL, NULL,
                                                      BS_TYPE_TOUCHSCREEN,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_touchscreen_slot_init (BsTouchscreenSlot *self)
{
}

BsTouchscreenSlot *
bs_touchscreen_slot_new (BsTouchscreen *touchscreen,
                         uint32_t       width,
                         uint32_t       height)
{
  g_autoptr (BsTouchscreenSlot) slot = NULL;

  slot = g_object_new (BS_TYPE_TOUCHSCREEN_SLOT,
                       "touchscreen", touchscreen,
                       NULL);
  slot->size = GRAPHENE_SIZE_INIT (width, height);

  return g_steal_pointer (&slot);
}

BsTouchscreen *
bs_touchscreen_slot_get_touchscreen (BsTouchscreenSlot *self)
{
  g_return_val_if_fail (BS_IS_TOUCHSCREEN_SLOT (self), NULL);

  return self->touchscreen;
}
