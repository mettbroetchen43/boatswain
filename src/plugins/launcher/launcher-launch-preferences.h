#pragma once

#include <gio/gio.h>
#include <adwaita.h>

#include "launcher-launch-action.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_LAUNCH_PREFERENCES (launcher_launch_preferences_get_type())
G_DECLARE_FINAL_TYPE (LauncherLaunchPreferences, launcher_launch_preferences, LAUNCHER, LAUNCH_PREFERENCES, AdwPreferencesGroup)

GtkWidget * launcher_launch_preferences_new (LauncherLaunchAction *launch_action);

G_END_DECLS
