/* launcher-plugin.c
 *
 * SPDX-FileCopyrightText: 2022 Emmanuele Bassi
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <libpeas/peas.h>

#include "bs-action-factory.h"
#include "launcher-action-factory.h"

G_MODULE_EXPORT void
launcher_plugin_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              BS_TYPE_ACTION_FACTORY,
                                              LAUNCHER_TYPE_ACTION_FACTORY);
}
