/* network-http-action-prefs.h
 *
 * Copyright 2023 Lorenzo Prosseda
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

#include "network-http-action.h"

G_BEGIN_DECLS

#define NETWORK_TYPE_HTTP_ACTION_PREFS (network_http_action_prefs_get_type())
G_DECLARE_FINAL_TYPE (NetworkHttpActionPrefs, network_http_action_prefs, NETWORK, HTTP_ACTION_PREFS, GtkBox)

GtkWidget * network_http_action_prefs_new (NetworkHttpAction *play_action);

void network_http_action_prefs_deserialize_settings (NetworkHttpActionPrefs *self,
                                                     JsonObject             *settings);

G_END_DECLS
