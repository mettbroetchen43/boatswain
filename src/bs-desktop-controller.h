/*
 * bs-desktop-controller.h
 *
 * Copyright 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#define BS_TYPE_DESKTOP_CONTROLLER (bs_desktop_controller_get_type())
G_DECLARE_FINAL_TYPE (BsDesktopController, bs_desktop_controller, BS, DESKTOP_CONTROLLER, GObject)

void bs_desktop_controller_acquire (BsDesktopController *self,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data);

gboolean bs_desktop_controller_acquire_finish (BsDesktopController  *self,
                                               GAsyncResult         *result,
                                               GError              **error);

void bs_desktop_controller_release (BsDesktopController *self);

void bs_desktop_controller_press_key (BsDesktopController *self,
                                      int                  keysym);

void bs_desktop_controller_release_key (BsDesktopController *self,
                                        int                  keysym);

G_END_DECLS
