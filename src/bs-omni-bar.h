/* bs-omni-bar.h
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

G_BEGIN_DECLS

#define BS_TYPE_OMNI_BAR (bs_omni_bar_get_type())
G_DECLARE_FINAL_TYPE (BsOmniBar, bs_omni_bar, BS, OMNI_BAR, GtkWidget)

GtkWidget  *bs_omni_bar_new         (void);

GtkPopover *bs_omni_bar_get_popover (BsOmniBar *self);

void        bs_omni_bar_set_popover (BsOmniBar  *self,
                                     GtkPopover *popover);

void        bs_omni_bar_add_prefix  (BsOmniBar *self,
                                     int        priority,
                                     GtkWidget *widget);

void        bs_omni_bar_add_suffix  (BsOmniBar *self,
                                     int        priority,
                                     GtkWidget *widget);

void        bs_omni_bar_remove      (BsOmniBar *self,
                                     GtkWidget *widget);

G_END_DECLS
