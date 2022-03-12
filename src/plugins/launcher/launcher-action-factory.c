#include "bs-action-factory.h"
#include "launcher-action-factory.h"
#include "launcher-launch-action.h"

#include <glib/gi18n.h>

struct _LauncherActionFactory
{
  PeasExtensionBase parent_instance;
};

static void bs_action_factory_iface_init (BsActionFactoryInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (LauncherActionFactory, launcher_action_factory, PEAS_TYPE_EXTENSION_BASE,
                               G_IMPLEMENT_INTERFACE (BS_TYPE_ACTION_FACTORY, bs_action_factory_iface_init));

static const BsActionInfo actions[] = {
  {
    .id = "launch-action",
    .icon_name = "application-x-executable-symbolic",
    .name = N_("Launch Application"),
    .description = NULL,
  },
};

static GList *
launcher_action_factory_list_actions (BsActionFactory *action_factory)
{
  GList *list = NULL;
  size_t i;

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    list = g_list_prepend (list, (gpointer) &actions[i]);

  return list;
}

static BsAction *
launcher_action_factory_create_action (BsActionFactory    *action_factory,
                                       BsStreamDeckButton *stream_deck_button,
                                       const BsActionInfo *action_info)
{
  if (g_strcmp0 (action_info->id, "launch-action") == 0)
    return launcher_launch_action_new (stream_deck_button);

  return NULL;
}

static void
bs_action_factory_iface_init (BsActionFactoryInterface *iface)
{
  iface->list_actions = launcher_action_factory_list_actions;
  iface->create_action = launcher_action_factory_create_action;
}

static void
launcher_action_factory_class_init (LauncherActionFactoryClass *klass)
{
}

static void
launcher_action_factory_init (LauncherActionFactory *self)
{
}
