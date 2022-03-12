/* launcher-action-factory.h
 *
 * SPDX-FileCopyrightText: 2022 Emmanuele Bassi
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <libpeas/peas.h>

G_BEGIN_DECLS

#define LAUNCHER_TYPE_ACTION_FACTORY (launcher_action_factory_get_type())
G_DECLARE_FINAL_TYPE (LauncherActionFactory, launcher_action_factory, LAUNCHER, ACTION_FACTORY, PeasExtensionBase)

G_END_DECLS
