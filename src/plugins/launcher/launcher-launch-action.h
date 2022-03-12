#pragma once

#include "bs-action.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_LAUNCH_ACTION (launcher_launch_action_get_type())
G_DECLARE_FINAL_TYPE (LauncherLaunchAction, launcher_launch_action, LAUNCHER, LAUNCH_ACTION, BsAction)

BsAction * launcher_launch_action_new (BsStreamDeckButton *stream_deck_button);

void launcher_launch_action_set_app (LauncherLaunchAction *self,
                                     GAppInfo             *app_info);

G_END_DECLS
