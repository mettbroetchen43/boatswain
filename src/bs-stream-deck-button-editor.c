/* bs-stream-deck-button-editor.c
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
#include "bs-action-factory.h"
#include "bs-application.h"
#include "bs-empty-action.h"
#include "bs-icon.h"
#include "bs-stream-deck-button.h"
#include "bs-stream-deck-button-editor.h"

#include <glib/gi18n.h>
#include <libpeas/peas.h>

struct _BsStreamDeckButtonEditor
{
  AdwBin parent_instance;

  AdwPreferencesGroup *action_preferences_group;
  AdwPreferencesGroup *actions_group;
  GtkColorChooser *background_color_button;
  GtkWidget *background_color_row;
  AdwPreferencesPage *button_preferences_page;
  GtkPicture *icon_picture;
  AdwLeaflet *leaflet;
  GtkWidget *remove_action_button;
  GtkStack *stack;

  BsStreamDeckButton *button;
  GtkWidget *action_preferences;

  gulong action_changed_id;
  gulong custom_icon_changed_id;
  gulong icon_changed_id;
};

static void on_action_row_activated_cb (GtkListBoxRow            *row,
                                        BsStreamDeckButtonEditor *self);

G_DEFINE_FINAL_TYPE (BsStreamDeckButtonEditor, bs_stream_deck_button_editor, ADW_TYPE_BIN)

enum
{
  PROP_0,
  PROP_BUTTON,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];


/*
 * Auxiliary methods
 */

static void
add_action_factory (BsStreamDeckButtonEditor *self,
                    BsActionFactory          *action_factory)
{
  g_autoptr (GList) actions = NULL;
  PeasPluginInfo *plugin_info;
  GtkWidget *expander_row;
  GtkWidget *image;
  GList *l;

  plugin_info = peas_extension_base_get_plugin_info (PEAS_EXTENSION_BASE (action_factory));

  expander_row = adw_expander_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (expander_row),
                                 peas_plugin_info_get_name (plugin_info));

  image = gtk_image_new_from_icon_name (peas_plugin_info_get_icon_name (plugin_info));
  adw_expander_row_add_prefix (ADW_EXPANDER_ROW (expander_row), image);

  adw_preferences_group_add (self->actions_group, expander_row);

  actions = bs_action_factory_list_actions (action_factory);
  for (l = actions; l; l = l->next)
    {
      const BsActionInfo *info;
      GtkWidget *image;
      GtkWidget *row;

      info = l->data;

      row = adw_action_row_new ();
      adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), info->name);
      adw_action_row_set_subtitle (ADW_ACTION_ROW (row), info->description);
      gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);
      g_object_set_data (G_OBJECT (row), "factory", action_factory);
      g_object_set_data (G_OBJECT (row), "action-info", (gpointer) info);
      g_signal_connect (row, "activated", G_CALLBACK (on_action_row_activated_cb), self);

      image = gtk_image_new_from_icon_name (info->icon_name);
      adw_action_row_add_prefix (ADW_ACTION_ROW (row), image);

      adw_expander_row_add_row (ADW_EXPANDER_ROW (expander_row), row);
    }
}

static void
setup_button (BsStreamDeckButtonEditor *self)
{
  BsIcon *custom_icon = bs_stream_deck_button_get_custom_icon (self->button);

  gtk_widget_set_visible (self->background_color_row, custom_icon != NULL);

  if (custom_icon)
    {
      gtk_color_chooser_set_rgba (self->background_color_button,
                                  bs_icon_get_background_color (custom_icon));
    }
}

static void
update_action_preferences_group (BsStreamDeckButtonEditor *self)
{
  GtkWidget *action_preferences;
  BsAction *action;

  action = self->button ? bs_stream_deck_button_get_action (self->button) : NULL;
  action_preferences = action ? bs_action_get_preferences (action) : NULL;

  gtk_widget_set_visible (self->remove_action_button,
                          action != NULL && !BS_IS_EMPTY_ACTION (action));

  if (self->action_preferences != action_preferences)
    {
      if (self->action_preferences)
        adw_preferences_group_remove (self->action_preferences_group, self->action_preferences);

      self->action_preferences = action_preferences;

      if (action_preferences)
        adw_preferences_group_add (self->action_preferences_group, action_preferences);

      gtk_widget_set_visible (GTK_WIDGET (self->action_preferences_group),
                              action_preferences != NULL);
    }
}

static void
update_icon (BsStreamDeckButtonEditor *self)
{
  BsIcon *icon = bs_stream_deck_button_get_icon (self->button);

  gtk_picture_set_paintable (self->icon_picture, GDK_PAINTABLE (icon));
}


/*
 * Callbacks
 */

static void
on_action_row_activated_cb (GtkListBoxRow            *row,
                            BsStreamDeckButtonEditor *self)
{
  g_autoptr (BsAction) action = NULL;
  BsActionFactory *action_factory;
  BsActionInfo *info;

  action_factory = g_object_get_data (G_OBJECT (row), "factory");
  info = g_object_get_data (G_OBJECT (row), "action-info");
  action = bs_action_factory_create_action (action_factory, self->button, info);

  bs_stream_deck_button_set_action (self->button, action);

  update_action_preferences_group (self);

  adw_leaflet_navigate (self->leaflet, ADW_NAVIGATION_DIRECTION_BACK);
}

static void
on_action_factory_added_cb (PeasExtensionSet *extension_set,
                            PeasPluginInfo   *plugin_info,
                            PeasExtension    *extension,
                            gpointer          user_data)
{
  BsStreamDeckButtonEditor *self = BS_STREAM_DECK_BUTTON_EDITOR (user_data);

  add_action_factory (self, BS_ACTION_FACTORY (extension));
}

static void
on_action_factory_removed_cb (PeasExtensionSet *extension_set,
                              PeasPluginInfo   *plugin_info,
                              PeasExtension    *extension,
                              gpointer          user_data)
{
}

static void
on_action_changed_cb (BsStreamDeckButton       *stream_deck_button,
                      GParamSpec               *pspec,
                      BsStreamDeckButtonEditor *self)
{
  update_action_preferences_group (self);
  setup_button (self);
  update_icon (self);
}

static void
on_background_color_button_color_set_cb (GtkColorButton           *color_button,
                                         BsStreamDeckButtonEditor *self)
{
  GdkRGBA background_color;
  BsIcon *icon;

  icon = bs_stream_deck_button_get_custom_icon (self->button);
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (color_button), &background_color);

  bs_icon_set_background_color (icon, &background_color);
}

static void
on_button_custom_icon_changed_cb (BsStreamDeckButton       *stream_deck_button,
                                  GParamSpec               *pspec,
                                  BsStreamDeckButtonEditor *self)
{
  setup_button (self);
}

static void
on_button_icon_changed_cb (BsStreamDeckButton       *stream_deck_button,
                           BsIcon                   *icon,
                           BsStreamDeckButtonEditor *self)
{
  setup_button (self);
  update_icon (self);
}

static void
on_file_chooser_native_response_cb (GtkNativeDialog          *native,
                                    int                       response,
                                    BsStreamDeckButtonEditor *self)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (native);
      g_autoptr (GtkIconPaintable) paintable = NULL;
      g_autoptr (GFile) file = NULL;
      BsIcon *icon;

      file = gtk_file_chooser_get_file (chooser);
      icon = bs_stream_deck_button_get_custom_icon (self->button);

      if (!icon)
        {
          g_autoptr (BsIcon) new_icon = NULL;
          g_autoptr (GError) error = NULL;

          new_icon = bs_icon_new_empty ();
          bs_stream_deck_button_set_custom_icon (self->button, new_icon, &error);

          if (error)
            {
              g_warning ("Error setting custom icon: %s", error->message);
              goto out;
            }

          icon = new_icon;
        }

      paintable = gtk_icon_paintable_new_for_file (file, 72, 1);
      bs_icon_set_paintable (icon, GDK_PAINTABLE (paintable));
    }

out:
  g_object_unref (native);
}

static void
on_custom_icon_button_clicked_cb (AdwPreferencesRow        *row,
                                  BsStreamDeckButtonEditor *self)
{
  GtkFileChooserNative *native;

  native = gtk_file_chooser_native_new (_("Select icon"),
                                        GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (self))),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        _("_Open"),
                                        _("_Cancel"));

  g_signal_connect (native, "response", G_CALLBACK (on_file_chooser_native_response_cb), self);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (native));
}

static void
on_go_previous_button_clicked_cb (GtkButton                *button,
                                  BsStreamDeckButtonEditor *self)
{
  adw_leaflet_navigate (self->leaflet, ADW_NAVIGATION_DIRECTION_BACK);
}

static void
on_remove_action_button_clicked_cb (GtkButton                *button,
                                    BsStreamDeckButtonEditor *self)
{
  g_autoptr (BsAction) empty_action = NULL;

  empty_action = bs_empty_action_new (self->button);
  bs_stream_deck_button_set_action (self->button, empty_action);
}

static void
on_select_action_row_activated_cb (GtkListBoxRow            *row,
                                   BsStreamDeckButtonEditor *self)
{
  adw_leaflet_navigate (self->leaflet, ADW_NAVIGATION_DIRECTION_FORWARD);
}


/*
 * GObject overrides
 */

static void
bs_stream_deck_button_editor_finalize (GObject *object)
{
  BsStreamDeckButtonEditor *self = (BsStreamDeckButtonEditor *)object;

  g_clear_signal_handler (&self->action_changed_id, self->button);
  g_clear_signal_handler (&self->custom_icon_changed_id, self->button);
  g_clear_signal_handler (&self->icon_changed_id, self->button);
  g_clear_object (&self->button);

  G_OBJECT_CLASS (bs_stream_deck_button_editor_parent_class)->finalize (object);
}

static void
bs_stream_deck_button_editor_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  BsStreamDeckButtonEditor *self = BS_STREAM_DECK_BUTTON_EDITOR (object);

  switch (prop_id)
    {
    case PROP_BUTTON:
      g_value_set_object (value, self->button);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_button_editor_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  BsStreamDeckButtonEditor *self = BS_STREAM_DECK_BUTTON_EDITOR (object);

  switch (prop_id)
    {
    case PROP_BUTTON:
      bs_stream_deck_button_editor_set_button (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_button_editor_class_init (BsStreamDeckButtonEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bs_stream_deck_button_editor_finalize;
  object_class->get_property = bs_stream_deck_button_editor_get_property;
  object_class->set_property = bs_stream_deck_button_editor_set_property;

  properties[PROP_BUTTON] = g_param_spec_object ("button", NULL, NULL,
                                                 BS_TYPE_STREAM_DECK_BUTTON,
                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/bs-stream-deck-button-editor.ui");

  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, action_preferences_group);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, actions_group);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, background_color_button);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, background_color_row);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, button_preferences_page);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, icon_picture);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, leaflet);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, remove_action_button);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, stack);

  gtk_widget_class_bind_template_callback (widget_class, on_background_color_button_color_set_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_custom_icon_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_go_previous_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_remove_action_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_select_action_row_activated_cb);
}

static void
bs_stream_deck_button_editor_init (BsStreamDeckButtonEditor *self)
{
  PeasExtensionSet *extension_set;
  GApplication *application;

  gtk_widget_init_template (GTK_WIDGET (self));

  application = g_application_get_default ();
  extension_set = bs_application_get_action_factory_set (BS_APPLICATION (application));

  peas_extension_set_foreach (extension_set,
                              (PeasExtensionSetForeachFunc) on_action_factory_added_cb,
                              self);

  g_signal_connect (extension_set, "extension-added", G_CALLBACK (on_action_factory_added_cb), self);
  g_signal_connect (extension_set, "extension-removed", G_CALLBACK (on_action_factory_removed_cb), self);
}

BsStreamDeckButton *
bs_stream_deck_button_editor_get_button (BsStreamDeckButtonEditor *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK_BUTTON_EDITOR (self), NULL);

  return self->button;
}

void
bs_stream_deck_button_editor_set_button (BsStreamDeckButtonEditor *self,
                                         BsStreamDeckButton       *button)
{
  g_return_if_fail (BS_IS_STREAM_DECK_BUTTON_EDITOR (self));

  g_clear_signal_handler (&self->custom_icon_changed_id, self->button);
  g_clear_signal_handler (&self->icon_changed_id, self->button);

  if (g_set_object (&self->button, button))
    {
      gtk_stack_set_visible_child_name (self->stack, button ? "button" : "empty");
      update_action_preferences_group (self);
      setup_button (self);
      update_icon (self);

      self->action_changed_id = g_signal_connect (button,
                                                  "notify::action",
                                                  G_CALLBACK (on_action_changed_cb),
                                                  self);

      self->custom_icon_changed_id = g_signal_connect (button,
                                                       "notify::custom-icon",
                                                       G_CALLBACK (on_button_custom_icon_changed_cb),
                                                       self);

      self->icon_changed_id = g_signal_connect (button,
                                                "icon-changed",
                                                G_CALLBACK (on_button_icon_changed_cb),
                                                self);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BUTTON]);
    }
}
