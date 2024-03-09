/*
 * bs-button-grid-region.c
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

#include "bs-button-grid-region.h"

#include "bs-device-region.h"
#include "bs-icon-renderer.h"
#include "bs-stream-deck.h"
#include "bs-stream-deck-button-private.h"

struct _BsButtonGridRegion
{
  BsDeviceRegion parent_instance;

  GListStore *buttons;
  BsIconRenderer *renderer;

  BsIconLayout icon_layout;
  unsigned int grid_columns;
};

G_DEFINE_FINAL_TYPE (BsButtonGridRegion, bs_button_grid_region, BS_TYPE_DEVICE_REGION)

enum {
  PROP_0,
  PROP_BUTTONS,
  PROP_GRID_COLUMNS,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

static void
bs_button_grid_region_finalize (GObject *object)
{
  BsButtonGridRegion *self = (BsButtonGridRegion *)object;

  g_clear_object (&self->buttons);
  g_clear_object (&self->renderer);

  G_OBJECT_CLASS (bs_button_grid_region_parent_class)->finalize (object);
}

static void
bs_button_grid_region_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BsButtonGridRegion *self = BS_BUTTON_GRID_REGION (object);

  switch (prop_id)
    {
    case PROP_BUTTONS:
      g_value_set_object (value, self->buttons);
      break;

    case PROP_GRID_COLUMNS:
      g_value_set_uint (value, self->grid_columns);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_button_grid_region_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BsButtonGridRegion *self = BS_BUTTON_GRID_REGION (object);

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
bs_button_grid_region_class_init (BsButtonGridRegionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_button_grid_region_finalize;
  object_class->get_property = bs_button_grid_region_get_property;
  object_class->set_property = bs_button_grid_region_set_property;

  properties[PROP_BUTTONS] = g_param_spec_object ("buttons", NULL, NULL,
                                                  G_TYPE_LIST_MODEL,
                                                  G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_GRID_COLUMNS] = g_param_spec_uint ("grid-columns", NULL, NULL,
                                                     1, G_MAXUINT, 1,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_button_grid_region_init (BsButtonGridRegion *self)
{
  self->buttons = g_list_store_new (BS_TYPE_STREAM_DECK_BUTTON);
  self->grid_columns = 1;
}

BsButtonGridRegion *
bs_button_grid_region_new (const char         *id,
                           BsStreamDeck       *stream_deck,
                           const BsIconLayout *icon_layout,
                           unsigned int        n_buttons,
                           unsigned int        grid_columns)
{
  return bs_button_grid_region_new_full (id,
                                         stream_deck,
                                         icon_layout,
                                         n_buttons,
                                         grid_columns,
                                         0, 0, 1, 1);
}

BsButtonGridRegion *
bs_button_grid_region_new_full (const char         *id,
                                BsStreamDeck       *stream_deck,
                                const BsIconLayout *icon_layout,
                                unsigned int        n_buttons,
                                unsigned int        grid_columns,
                                unsigned int        column,
                                unsigned int        row,
                                unsigned int        column_span,
                                unsigned int        row_span)
{
  g_autoptr(BsButtonGridRegion) self = NULL;

  g_assert (BS_IS_STREAM_DECK (stream_deck));
  g_assert (icon_layout != NULL);

  self = g_object_new (BS_TYPE_BUTTON_GRID_REGION,
                       "id", id,
                       "stream-deck", stream_deck,
                       "grid-columns", grid_columns,
                       "column", column,
                       "row", row,
                       "column-span", column_span,
                       "row-span", row_span,
                       NULL);

  /* TODO: make it a construct-only property */
  self->icon_layout = *icon_layout;

  self->renderer = bs_icon_renderer_new (&self->icon_layout);

  for (unsigned int i = 0; i < n_buttons; i++)
    {
      g_autoptr(BsStreamDeckButton) button = NULL;

      button = bs_stream_deck_button_new (stream_deck,
                                          i,
                                          icon_layout->width,
                                          icon_layout->height);

      g_list_store_append (self->buttons, button);
    }

  return g_steal_pointer (&self);
}

GListModel *
bs_button_grid_region_get_buttons (BsButtonGridRegion *self)
{
  g_return_val_if_fail (BS_IS_BUTTON_GRID_REGION (self), NULL);

  return G_LIST_MODEL (self->buttons);
}

unsigned int
bs_button_grid_region_get_grid_columns (BsButtonGridRegion *self)
{
  g_return_val_if_fail (BS_IS_BUTTON_GRID_REGION (self), 0);

  return self->grid_columns;
}
