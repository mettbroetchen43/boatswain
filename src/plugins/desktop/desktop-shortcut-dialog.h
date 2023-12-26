/*
 * desktop-shortcut-dialog.h
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

#include <adwaita.h>

G_BEGIN_DECLS

#define DESKTOP_TYPE_SHORTCUT_DIALOG (desktop_shortcut_dialog_get_type())
G_DECLARE_FINAL_TYPE (DesktopShortcutDialog, desktop_shortcut_dialog, DESKTOP, SHORTCUT_DIALOG, AdwWindow)

DesktopShortcutDialog * desktop_shortcut_dialog_new (GdkModifierType modifiers,
                                                     unsigned int    keysym);

void desktop_shortcut_dialog_run (DesktopShortcutDialog *self,
                                  GtkWindow             *parent,
                                  GCancellable          *cancellable,
                                  GAsyncReadyCallback    callback,
                                  gpointer               user_data);

gboolean desktop_shortcut_dialog_run_finish (DesktopShortcutDialog  *self,
                                             GAsyncResult           *result,
                                             GdkModifierType        *out_modifiers,
                                             unsigned int           *out_keysym,
                                             GError                **error);

G_END_DECLS
