/* dummy-action-1.c
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

#include "bs-action.h"
#include "bs-icon.h"
#include "dummy-action-1.h"

struct _DummyAction1
{
  BsAction parent_instance;

  BsIcon *icon;
};

G_DEFINE_FINAL_TYPE (DummyAction1, dummy_action_1, BS_TYPE_ACTION)


/*
 * Auxiliary methods
 */

static void
set_icon (DummyAction1 *self,
          const char   *icon_name)
{
  g_autoptr (GtkIconPaintable) icon_paintable = NULL;
  GtkIconTheme *icon_theme;

  icon_theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());
  icon_paintable = gtk_icon_theme_lookup_icon (icon_theme,
                                               icon_name,
                                               NULL,
                                               72,
                                               1,
                                               GTK_TEXT_DIR_RTL,
                                               0);

  bs_icon_set_paintable (self->icon, GDK_PAINTABLE (icon_paintable));
}


/*
 * BsAction overrides
 */

static void
dummy_action_1_activate (BsAction *action)
{
  DummyAction1 *self = DUMMY_ACTION_1 (action);

  set_icon (self, "face-cool-symbolic");
  bs_icon_set_text (self->icon, "Hi!");
}

static void
dummy_action_1_deactivate (BsAction *action)
{
  DummyAction1 *self = DUMMY_ACTION_1 (action);

  set_icon (self, "face-tired-symbolic");
  bs_icon_set_text (self->icon, "Bye!");
}

static BsIcon *
dummy_action_1_get_icon (BsAction *action)
{
  DummyAction1 *self = DUMMY_ACTION_1 (action);

  return self->icon;
}

static AdwPreferencesGroup *
dummy_action_1_get_preferences (BsAction *action)
{
  GtkWidget *group;

  group = adw_preferences_group_new ();
  adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (group), "Dummy Plugin 1");

  return ADW_PREFERENCES_GROUP (group);
}


/*
 * GObject overrides
 */

static void
dummy_action_1_finalize (GObject *object)
{
  DummyAction1 *self = (DummyAction1 *)object;

  g_clear_object (&self->icon);

  G_OBJECT_CLASS (dummy_action_1_parent_class)->finalize (object);
}

static void
dummy_action_1_class_init (DummyAction1Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BsActionClass *action_class = BS_ACTION_CLASS (klass);

  object_class->finalize = dummy_action_1_finalize;

  action_class->activate = dummy_action_1_activate;
  action_class->deactivate = dummy_action_1_deactivate;
  action_class->get_icon = dummy_action_1_get_icon;
  action_class->get_preferences = dummy_action_1_get_preferences;
}

static void
dummy_action_1_init (DummyAction1 *self)
{
  self->icon = bs_icon_new_empty ();
  set_icon (self, "emoji-body-symbolic");
}
