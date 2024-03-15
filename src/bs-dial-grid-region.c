/*
 * bs-dial-grid-region.c
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

#include "bs-dial-grid-region.h"

#include "bs-dial-private.h"
#include "bs-stream-deck.h"

struct _BsDialGridRegion
{
  BsDeviceRegion parent_instance;

  GListStore *dials;

  unsigned int grid_columns;
};

G_DEFINE_FINAL_TYPE (BsDialGridRegion, bs_dial_grid_region, BS_TYPE_DEVICE_REGION)

enum {
  PROP_0,
  PROP_DIALS,
  PROP_GRID_COLUMNS,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

static void
bs_dial_grid_region_finalize (GObject *object)
{
  BsDialGridRegion *self = (BsDialGridRegion *)object;

  g_clear_object (&self->dials);

  G_OBJECT_CLASS (bs_dial_grid_region_parent_class)->finalize (object);
}

static void
bs_dial_grid_region_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BsDialGridRegion *self = BS_DIAL_GRID_REGION (object);

  switch (prop_id)
    {
    case PROP_DIALS:
      g_value_set_object (value, self->dials);
      break;

    case PROP_GRID_COLUMNS:
      g_value_set_uint (value, self->grid_columns);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_dial_grid_region_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BsDialGridRegion *self = BS_DIAL_GRID_REGION (object);

  switch (prop_id)
    {
    case PROP_GRID_COLUMNS:
      self->grid_columns = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_dial_grid_region_class_init (BsDialGridRegionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_dial_grid_region_finalize;
  object_class->get_property = bs_dial_grid_region_get_property;
  object_class->set_property = bs_dial_grid_region_set_property;

  properties[PROP_DIALS] = g_param_spec_object ("dials", NULL, NULL,
                                                G_TYPE_LIST_MODEL,
                                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_GRID_COLUMNS] = g_param_spec_uint ("grid-columns", NULL, NULL,
                                                     1, G_MAXUINT, 1,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_dial_grid_region_init (BsDialGridRegion *self)
{
  self->dials = g_list_store_new (BS_TYPE_DIAL);
  self->grid_columns = 1;
}

BsDialGridRegion *
bs_dial_grid_region_new (const char   *id,
                         BsStreamDeck *stream_deck,
                         unsigned int  n_dials,
                         unsigned int  grid_columns,
                         unsigned int  column,
                         unsigned int  row,
                         unsigned int  column_span,
                         unsigned int  row_span)
{
  g_autoptr (BsDialGridRegion) self = NULL;

  g_assert (BS_IS_STREAM_DECK (stream_deck));
  g_assert (n_dials > 0);

  self = g_object_new (BS_TYPE_DIAL_GRID_REGION,
                       "id", id,
                       "stream-deck", stream_deck,
                       "grid-columns", grid_columns,
                       "column", column,
                       "row", row,
                       "column-span", column_span,
                       "row-span", row_span,
                       NULL);

  for (unsigned int i = 0; i < n_dials; i++)
    {
      g_autoptr (BsDial) dial = bs_dial_new (stream_deck, i);
      g_list_store_append (self->dials, dial);
    }

  return g_steal_pointer (&self);
}

GListModel *
bs_dial_grid_region_get_dials (BsDialGridRegion *self)
{
  g_return_val_if_fail (BS_IS_DIAL_GRID_REGION (self), NULL);

  return G_LIST_MODEL (self->dials);
}

unsigned int
bs_dial_grid_region_get_grid_columns (BsDialGridRegion *self)
{
  g_return_val_if_fail (BS_IS_DIAL_GRID_REGION (self), 0);

  return self->grid_columns;
}
