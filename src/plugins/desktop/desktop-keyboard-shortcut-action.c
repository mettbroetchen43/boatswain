/*
 * desktop-keyboard-shortcut-action.c
 *
 * Copyright 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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
#include "desktop-keyboard-shortcut-action.h"
#include "desktop-shortcut-dialog.h"

#include <adwaita.h>
#include <glib/gi18n.h>

#if !GTK_CHECK_VERSION (4, 13, 4)
#define GDK_NO_MODIFIER_MASK 0
#endif

struct _DesktopKeyboardShortcutAction
{
  BsAction parent_instance;

  BsDesktopController *desktop_controller;
  GtkListBoxRow *row;

  GdkModifierType modifiers;
  unsigned int keysym;

  GCancellable *cancellable;
  gboolean acquired;
};

G_DEFINE_FINAL_TYPE (DesktopKeyboardShortcutAction, desktop_keyboard_shortcut_action, BS_TYPE_ACTION)

enum
{
  PROP_0,
  PROP_DESKTOP_CONTROLLER,
  N_PROPS
};

static GParamSpec *properties [N_PROPS] = { NULL, };


/*
 * Auxiliary functions
 */

static void
update_icon_text (DesktopKeyboardShortcutAction *self)
{
  g_autofree char *accelerator = NULL;

  if (self->keysym != 0 && self->modifiers != 0)
    accelerator = gtk_accelerator_get_label (self->keysym, self->modifiers);

  bs_icon_set_text (bs_action_get_icon (BS_ACTION (self)), accelerator);
}

static void
update_shortcut_row (DesktopKeyboardShortcutAction *self,
                     AdwActionRow                  *row)
{
  g_autofree char *accelerator = NULL;
  GtkWidget *shortcut_label;

  shortcut_label = g_object_get_data (G_OBJECT (row), "shortcut-label");

  if (self->keysym != 0 && self->modifiers != 0)
    accelerator = gtk_accelerator_get_label (self->keysym, self->modifiers);
  else
    accelerator = g_strdup ("");

  gtk_label_set_text (GTK_LABEL (shortcut_label), accelerator);
}


/*
 * Callbacks
 */

static void
on_desktop_controller_acquired_cb (GObject      *source_object,
                                   GAsyncResult *result,
                                   gpointer      user_data)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (user_data);
  g_autoptr (GError) error = NULL;

  g_clear_object (&self->cancellable);

  bs_desktop_controller_acquire_finish (BS_DESKTOP_CONTROLLER (source_object),
                                        result,
                                        &error);

  if (error)
    return;

  self->acquired = TRUE;

  if (self->row)
    gtk_list_box_row_set_activatable (self->row, TRUE);
}

static void
on_shortcut_dialog_run_finished_cb (GObject      *source_object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (user_data);
  g_autoptr (GError) error = NULL;
  GdkModifierType modifiers;
  unsigned int keysym;

  desktop_shortcut_dialog_run_finish (DESKTOP_SHORTCUT_DIALOG (source_object),
                                      result,
                                      &modifiers,
                                      &keysym,
                                      &error);

  if (error)
    return;

  self->modifiers = modifiers;
  self->keysym = keysym;

  update_shortcut_row (self, ADW_ACTION_ROW (self->row));
  update_icon_text (self);

  bs_action_changed (BS_ACTION (self));
}

static void
on_row_activated_cb (AdwActionRow                  *row,
                     DesktopKeyboardShortcutAction *self)
{
  g_autoptr (DesktopShortcutDialog) shortcut_dialog = NULL;

  g_assert (self->acquired);
  g_assert (self->cancellable == NULL);

  shortcut_dialog = desktop_shortcut_dialog_new (self->modifiers, self->keysym);
  desktop_shortcut_dialog_run (shortcut_dialog,
                               GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (row))),
                               self->cancellable,
                               on_shortcut_dialog_run_finished_cb,
                               self);
}


/*
 * BsAction overrides
 */

static void
desktop_keyboard_shortcut_action_activate (BsAction *action)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (action);

  if (self->modifiers & GDK_SUPER_MASK)
    bs_desktop_controller_press_key (self->desktop_controller, GDK_KEY_Super_L);

  if (self->modifiers & GDK_CONTROL_MASK)
    bs_desktop_controller_press_key (self->desktop_controller, GDK_KEY_Control_L);

  if (self->modifiers & GDK_SHIFT_MASK)
    bs_desktop_controller_press_key (self->desktop_controller, GDK_KEY_Shift_L);

  if (self->modifiers & GDK_ALT_MASK)
    bs_desktop_controller_press_key (self->desktop_controller, GDK_KEY_Alt_L);

  if (self->keysym != 0)
    bs_desktop_controller_press_key (self->desktop_controller, self->keysym);
}

static void
desktop_keyboard_shortcut_action_deactivate (BsAction *action)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (action);

  if (self->keysym != 0)
    bs_desktop_controller_release_key (self->desktop_controller, self->keysym);

  if (self->modifiers & GDK_ALT_MASK)
    bs_desktop_controller_release_key (self->desktop_controller, GDK_KEY_Alt_L);

  if (self->modifiers & GDK_SHIFT_MASK)
    bs_desktop_controller_release_key (self->desktop_controller, GDK_KEY_Shift_L);

  if (self->modifiers & GDK_CONTROL_MASK)
    bs_desktop_controller_release_key (self->desktop_controller, GDK_KEY_Control_L);

  if (self->modifiers & GDK_SUPER_MASK)
    bs_desktop_controller_release_key (self->desktop_controller, GDK_KEY_Super_L);
}

static JsonNode *
desktop_keyboard_shortcut_action_serialize_settings (BsAction *action)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (action);
  g_autoptr (JsonBuilder) builder = NULL;

  builder = json_builder_new ();

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "modifiers");
  json_builder_add_int_value (builder, self->modifiers);

  json_builder_set_member_name (builder, "keysym");
  json_builder_add_int_value (builder, self->keysym);

  json_builder_end_object (builder);

  return json_builder_get_root (builder);
}

static void
desktop_keyboard_shortcut_action_deserialize_settings (BsAction   *action,
                                                       JsonObject *settings)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (action);

  self->modifiers = json_object_get_int_member_with_default (settings, "modifiers", GDK_NO_MODIFIER_MASK);
  self->keysym = json_object_get_int_member_with_default (settings, "keysym", 0);

  update_icon_text (self);
}

static GtkWidget *
desktop_keyboard_shortcut_action_get_preferences (BsAction *action)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (action);
  GtkWidget *shortcut_label;
  GtkWidget *image;
  GtkWidget *group;
  GtkWidget *row;

  group = adw_preferences_group_new ();

  row = adw_action_row_new ();
  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), self->acquired);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _("Keyboard Shortcut"));
  adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), row);
  g_signal_connect (row, "activated", G_CALLBACK (on_row_activated_cb), self);

  self->row = GTK_LIST_BOX_ROW (row);
  g_object_add_weak_pointer (G_OBJECT (row), (gpointer *) &self->row);

  shortcut_label = gtk_label_new ("");
  gtk_widget_set_valign (shortcut_label, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (ADW_ACTION_ROW (row), shortcut_label);
  g_object_set_data (G_OBJECT (row), "shortcut-label", shortcut_label);

  image = gtk_image_new_from_icon_name ("go-next-symbolic");
  gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (ADW_ACTION_ROW (row), image);

  update_shortcut_row (self, ADW_ACTION_ROW (row));

  return group;
}


/*
 * GObject overrides
 */

static void
desktop_keyboard_shortcut_action_dispose (GObject *object)
{
  DesktopKeyboardShortcutAction *self = (DesktopKeyboardShortcutAction *)object;

  if (self->acquired)
    bs_desktop_controller_release (self->desktop_controller);

  g_cancellable_cancel (self->cancellable);

  g_clear_object (&self->desktop_controller);
  g_clear_object (&self->cancellable);

  G_OBJECT_CLASS (desktop_keyboard_shortcut_action_parent_class)->dispose (object);
}

static void
desktop_keyboard_shortcut_action_constructed (GObject *object)
{
  DesktopKeyboardShortcutAction *self = (DesktopKeyboardShortcutAction *)object;

  G_OBJECT_CLASS (desktop_keyboard_shortcut_action_parent_class)->constructed (object);

  self->cancellable = g_cancellable_new ();
  bs_desktop_controller_acquire (self->desktop_controller,
                                 self->cancellable,
                                 on_desktop_controller_acquired_cb,
                                 self);
}

static void
desktop_keyboard_shortcut_action_get_property (GObject    *object,
                                               guint       prop_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (object);

  switch (prop_id)
    {
    case PROP_DESKTOP_CONTROLLER:
      g_value_set_object (value, self->desktop_controller);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
desktop_keyboard_shortcut_action_set_property (GObject      *object,
                                               guint         prop_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  DesktopKeyboardShortcutAction *self = DESKTOP_KEYBOARD_SHORTCUT_ACTION (object);

  switch (prop_id)
    {
    case PROP_DESKTOP_CONTROLLER:
      g_assert (self->desktop_controller == NULL);
      self->desktop_controller = g_value_dup_object (value);
      g_assert (self->desktop_controller != NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
desktop_keyboard_shortcut_action_class_init (DesktopKeyboardShortcutActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BsActionClass *action_class = BS_ACTION_CLASS (klass);

  object_class->dispose = desktop_keyboard_shortcut_action_dispose;
  object_class->constructed = desktop_keyboard_shortcut_action_constructed;
  object_class->get_property = desktop_keyboard_shortcut_action_get_property;
  object_class->set_property = desktop_keyboard_shortcut_action_set_property;

  action_class->activate = desktop_keyboard_shortcut_action_activate;
  action_class->deactivate = desktop_keyboard_shortcut_action_deactivate;
  action_class->serialize_settings = desktop_keyboard_shortcut_action_serialize_settings;
  action_class->deserialize_settings = desktop_keyboard_shortcut_action_deserialize_settings;
  action_class->get_preferences = desktop_keyboard_shortcut_action_get_preferences;

  properties[PROP_DESKTOP_CONTROLLER] =
    g_param_spec_object ("desktop-controller", "", "",
                         BS_TYPE_DESKTOP_CONTROLLER,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
desktop_keyboard_shortcut_action_init (DesktopKeyboardShortcutAction *self)
{
  bs_icon_set_icon_name (bs_action_get_icon (BS_ACTION (self)),
                         "preferences-desktop-keyboard-shortcuts-symbolic");
}

BsAction *
desktop_keyboard_shortcut_action_new (BsDesktopController *desktop_controller)
{
  return g_object_new (DESKTOP_TYPE_KEYBOARD_SHORTCUT_ACTION,
                       "desktop-controller", desktop_controller,
                       NULL);
}
