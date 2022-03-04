/* default-switch-profile-action.c
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

#include "bs-icon.h"
#include "bs-profile.h"
#include "bs-stream-deck.h"
#include "bs-stream-deck-button.h"
#include "default-switch-profile-action.h"

#include <glib/gi18n.h>

struct _DefaultSwitchProfileAction
{
  BsAction parent_instance;

  BsProfile *profile;
};

G_DEFINE_FINAL_TYPE (DefaultSwitchProfileAction, default_switch_profile_action, BS_TYPE_ACTION)


/*
 * Auxiliary methods
 */

static BsProfile *
get_profile_from_stream_deck (DefaultSwitchProfileAction *self,
                              const char                 *profile_id)
{
  BsStreamDeckButton *stream_deck_button;
  BsStreamDeck *stream_deck;
  GListModel *profiles;
  unsigned int i;

  stream_deck_button = bs_action_get_stream_deck_button (BS_ACTION (self));
  stream_deck = bs_stream_deck_button_get_stream_deck (stream_deck_button);
  profiles = bs_stream_deck_get_profiles (stream_deck);

  for (i = 0; i < g_list_model_get_n_items (profiles); i++)
    {
      g_autoptr (BsProfile) profile = g_list_model_get_item (profiles, i);

      if (g_strcmp0 (bs_profile_get_id (profile), profile_id) == 0)
        return profile;
    }

  return bs_stream_deck_get_active_profile (stream_deck);
}

static void
set_active_profile (DefaultSwitchProfileAction *self,
                    BsProfile                  *profile)
{
  if (g_set_object (&self->profile, profile))
    {
      BsIcon *icon = bs_action_get_icon (BS_ACTION (self));

      bs_icon_set_text (icon, bs_profile_get_name (profile));
      bs_action_changed (BS_ACTION (self));
    }
}


/*
 * Callbacks
 */

static void
on_combo_row_selected_changed_cb (AdwComboRow                *combo_row,
                                  GParamSpec                 *pspec,
                                  DefaultSwitchProfileAction *self)
{
  set_active_profile (self, adw_combo_row_get_selected_item (combo_row));
}

static void
on_profiles_items_changed_cb (GListModel                 *list,
                              unsigned int                position,
                              unsigned int                removed,
                              unsigned int                added,
                              DefaultSwitchProfileAction *self)
{
  const char *profile_id = bs_profile_get_id (self->profile);

  set_active_profile (self, get_profile_from_stream_deck (self, profile_id));
}


/*
 * BsAction overrides
 */

static void
default_switch_profile_action_activate (BsAction *action)
{
  DefaultSwitchProfileAction *self;
  BsStreamDeckButton *stream_deck_button;
  BsStreamDeck *stream_deck;

  self = DEFAULT_SWITCH_PROFILE_ACTION (action);
  stream_deck_button = bs_action_get_stream_deck_button (BS_ACTION (self));
  stream_deck = bs_stream_deck_button_get_stream_deck (stream_deck_button);

  bs_stream_deck_load_profile (stream_deck, self->profile);
}

static GtkWidget *
default_switch_profile_action_get_preferences (BsAction *action)
{
  DefaultSwitchProfileAction *self = DEFAULT_SWITCH_PROFILE_ACTION (action);
  BsStreamDeckButton *stream_deck_button;
  BsStreamDeck *stream_deck;
  GListModel *profiles;
  GtkWidget *row;
  unsigned int position;

  stream_deck_button = bs_action_get_stream_deck_button (BS_ACTION (self));
  stream_deck = bs_stream_deck_button_get_stream_deck (stream_deck_button);
  profiles = bs_stream_deck_get_profiles (stream_deck);

  row = adw_combo_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _("Profile"));
  adw_combo_row_set_expression (ADW_COMBO_ROW (row),
                                gtk_property_expression_new (BS_TYPE_PROFILE, NULL, "name"));
  adw_combo_row_set_model (ADW_COMBO_ROW (row), profiles);
  g_signal_connect (row, "notify::selected", G_CALLBACK (on_combo_row_selected_changed_cb), self);

  if (!self->profile || !g_list_store_find (G_LIST_STORE (profiles), self->profile, &position))
    position = 0;

  adw_combo_row_set_selected (ADW_COMBO_ROW (row), position);

  return row;
}

static JsonNode *
default_switch_profile_action_serialize_settings (BsAction *action)
{
  DefaultSwitchProfileAction *self = DEFAULT_SWITCH_PROFILE_ACTION (action);
  g_autoptr (JsonBuilder) builder = NULL;

  builder = json_builder_new ();

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "profile-id");
  if (self->profile)
    json_builder_add_string_value (builder, bs_profile_get_id (self->profile));
  else
    json_builder_add_null_value (builder);

  json_builder_end_object (builder);

  return json_builder_get_root (builder);
}

static void
default_switch_profile_action_deserialize_settings (BsAction   *action,
                                                    JsonObject *object)
{
  DefaultSwitchProfileAction *self = DEFAULT_SWITCH_PROFILE_ACTION (action);
  const char *profile_id;

  profile_id = json_object_get_string_member (object, "profile-id");

  set_active_profile (self, get_profile_from_stream_deck (self, profile_id));
}


/*
 * GObject overrides
 */

static void
default_switch_profile_action_finalize (GObject *object)
{
  DefaultSwitchProfileAction *self = (DefaultSwitchProfileAction *)object;

  g_clear_object (&self->profile);

  G_OBJECT_CLASS (default_switch_profile_action_parent_class)->finalize (object);
}

static void
default_switch_profile_action_constructed (GObject *object)
{
  DefaultSwitchProfileAction *self = (DefaultSwitchProfileAction *)object;
  BsStreamDeckButton *stream_deck_button;
  BsStreamDeck *stream_deck;
  GListModel *profiles;
  BsIcon *icon;

  G_OBJECT_CLASS (default_switch_profile_action_parent_class)->constructed (object);

  stream_deck_button = bs_action_get_stream_deck_button (BS_ACTION (self));
  stream_deck = bs_stream_deck_button_get_stream_deck (stream_deck_button);
  profiles = bs_stream_deck_get_profiles (stream_deck);
  g_signal_connect_object (profiles,
                           "items-changed",
                           G_CALLBACK (on_profiles_items_changed_cb),
                           self,
                           0);

  icon = bs_action_get_icon (BS_ACTION (self));
  bs_icon_set_margin (icon, 18);
  bs_icon_set_icon_name (icon, "preferences-desktop-apps-symbolic");
}


static void
default_switch_profile_action_class_init (DefaultSwitchProfileActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BsActionClass *action_class = BS_ACTION_CLASS (klass);

  object_class->finalize = default_switch_profile_action_finalize;
  object_class->constructed = default_switch_profile_action_constructed;

  action_class->activate = default_switch_profile_action_activate;
  action_class->get_preferences = default_switch_profile_action_get_preferences;
  action_class->serialize_settings = default_switch_profile_action_serialize_settings;
  action_class->deserialize_settings = default_switch_profile_action_deserialize_settings;
}

static void
default_switch_profile_action_init (DefaultSwitchProfileAction *self)
{
}

BsAction *
default_switch_profile_action_new (BsStreamDeckButton *stream_deck_button)
{
  return g_object_new (DEFAULT_TYPE_SWITCH_PROFILE_ACTION,
                       "stream-deck-button", stream_deck_button,
                       NULL);
}

BsProfile *
default_switch_profile_action_get_profile (DefaultSwitchProfileAction *self)
{
  g_return_val_if_fail (DEFAULT_IS_SWITCH_PROFILE_ACTION (self), NULL);

  return self->profile;
}
