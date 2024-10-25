/*
 * bs-selection-controller.c
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

#include "bs-selection-controller.h"

struct _BsSelectionController
{
  GObject parent_instance;

  gboolean selected;
  gpointer owner;
  gpointer item;
};

enum
{
  SELECTION_CHANGED,
  N_SIGNALS,
};

static guint signals [N_SIGNALS];

G_DEFINE_FINAL_TYPE (BsSelectionController, bs_selection_controller, G_TYPE_OBJECT)

static void
bs_selection_controller_class_init (BsSelectionControllerClass *klass)
{
  signals[SELECTION_CHANGED] = g_signal_new ("selection-changed",
                                             BS_TYPE_SELECTION_CONTROLLER,
                                             G_SIGNAL_RUN_LAST,
                                             0, NULL, NULL, NULL,
                                             G_TYPE_NONE,
                                             0);
}

static void
bs_selection_controller_init (BsSelectionController *self)
{

}

BsSelectionController *
bs_selection_controller_new (void)
{
  return g_object_new (BS_TYPE_SELECTION_CONTROLLER, NULL);
}

gboolean
bs_selection_controller_get_selection (BsSelectionController *self,
                                       gpointer              *out_owner,
                                       gpointer              *out_item)
{
  g_assert (BS_IS_SELECTION_CONTROLLER (self));

  if (!self->selected)
    return FALSE;

  if (out_owner)
    *out_owner = self->owner;
  if (out_item)
    *out_item = self->item;

  return TRUE;
}

void
bs_selection_controller_set_selection (BsSelectionController *self,
                                       gpointer               owner,
                                       gpointer               item)
{
  g_assert (BS_IS_SELECTION_CONTROLLER (self));
  g_assert (owner != NULL && item != NULL);

  if (self->owner == owner && self->item == item)
    return;

  self->selected = TRUE;
  self->owner = owner;
  self->item = item;

  g_signal_emit (self, signals[SELECTION_CHANGED], 0);
}

void
bs_selection_controller_unselect (BsSelectionController *self)
{
  g_assert (BS_IS_SELECTION_CONTROLLER (self));

  if (!self->selected)
    return;

  self->selected = FALSE;
  self->owner = NULL;
  self->item = NULL;

  g_signal_emit (self, signals[SELECTION_CHANGED], 0);
}
