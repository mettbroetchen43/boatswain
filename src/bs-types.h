/* bs-types.h
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

#include <glib.h>

G_BEGIN_DECLS

typedef enum _BsIconComposeFlags BsIconComposeFlags;
typedef enum _BsIconFormat BsIconFormat;
typedef enum _BsIconRendererFlags BsIconRendererFlags;

typedef struct _BsAction BsAction;
typedef struct _BsActionFactory BsActionFactory;
typedef struct _BsApplication BsApplication;
typedef struct _BsDeviceManager BsDeviceManager;
typedef struct _BsEmptyAction BsEmptyAction;
typedef struct _BsIcon BsIcon;
typedef struct _BsIconLayout BsIconLayout;
typedef struct _BsIconRenderer BsIconRenderer;
typedef struct _BsPage BsPage;
typedef struct _BsPageItem BsPageItem;
typedef struct _BsProfile BsProfile;
typedef struct _BsStreamDeck BsStreamDeck;
typedef struct _BsStreamDeckButton BsStreamDeckButton;
typedef struct _BsStreamDeckEditor BsStreamDeckEditor;
typedef struct _BsWindow BsWindow;

G_END_DECLS
