/* launcher-launch-action.h
 *
 * Copyright 2022 Emmanuele Bassi
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

#include <gio/gio.h>
#include <adwaita.h>

#include "launcher-launch-action.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_LAUNCH_PREFERENCES (launcher_launch_preferences_get_type())
G_DECLARE_FINAL_TYPE (LauncherLaunchPreferences, launcher_launch_preferences, LAUNCHER, LAUNCH_PREFERENCES, AdwPreferencesGroup)

GtkWidget * launcher_launch_preferences_new (LauncherLaunchAction *launch_action);

G_END_DECLS
