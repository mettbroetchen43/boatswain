/*
 * bs-dial.c
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

#include "bs-dial.h"

#include "bs-stream-deck.h"

struct _BsDial
{
  GObject parent_instance;

  gboolean pressed;
  double value;

  BsStreamDeck *stream_deck;
  uint8_t position;
};

G_DEFINE_FINAL_TYPE (BsDial, bs_dial, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_PRESSED,
  PROP_VALUE,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];


/*
 * GObject overrides
 */

static void
bs_dial_finalize (GObject *object)
{
  //BsDial *self = (BsDial *)object;

  G_OBJECT_CLASS (bs_dial_parent_class)->finalize (object);
}

static void
bs_dial_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  BsDial *self = BS_DIAL (object);

  switch (prop_id)
    {
    case PROP_PRESSED:
      g_value_set_boolean (value, self->pressed);
      break;

    case PROP_VALUE:
      g_value_set_double (value, self->value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_dial_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  //BsDial *self = BS_DIAL (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_dial_class_init (BsDialClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_dial_finalize;
  object_class->get_property = bs_dial_get_property;
  object_class->set_property = bs_dial_set_property;

  properties[PROP_PRESSED] = g_param_spec_boolean ("pressed", NULL, NULL,
                                                   FALSE,
                                                   G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_VALUE] = g_param_spec_double ("value", NULL, NULL,
                                                0.0, 1.0, 0.0,
                                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_dial_init (BsDial *self)
{
}

BsDial *
bs_dial_new (BsStreamDeck *stream_deck,
             uint8_t       position)
{
  g_autoptr (BsDial) self = NULL;

  self = g_object_new (BS_TYPE_DIAL, NULL);
  self->stream_deck = stream_deck;
  self->position = position;

  return g_steal_pointer (&self);
}

gboolean
bs_dial_get_pressed (BsDial *self)
{
  g_return_val_if_fail (BS_IS_DIAL (self), FALSE);

  return self->pressed;
}

void
bs_dial_set_pressed (BsDial   *self,
                     gboolean  pressed)
{
  g_assert (BS_IS_DIAL (self));

  if (self->pressed == pressed)
    return;

  self->pressed = pressed;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRESSED]);
}

double
bs_dial_get_value (BsDial *self)
{
  g_return_val_if_fail (BS_IS_DIAL (self), 0.0);

  return self->value;
}

void
bs_dial_set_value (BsDial *self,
                   double  value)
{
  g_assert (BS_IS_DIAL (self));
  g_assert (value >= 0.0 && value <= 1.0);

  if (G_APPROX_VALUE (self->value, value, DBL_EPSILON))
    return;

  self->value = value;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);
}