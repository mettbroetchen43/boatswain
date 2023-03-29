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
#include "bs-page.h"
#include "bs-page-item.h"
#include "bs-stream-deck.h"
#include "bs-stream-deck-button.h"
#include "bs-stream-deck-button-editor.h"

#include <glib/gi18n.h>
#include <libpeas.h>

struct _BsStreamDeckButtonEditor
{
  AdwBin parent_instance;

  AdwPreferencesGroup *action_preferences_group;
  GtkListBox *actions_listbox;
  GtkColorChooser *background_color_button;
  GtkStringList *builtin_icons_stringlist;
  AdwPreferencesPage *button_preferences_page;
  GtkMenuButton *custom_icon_menubutton;
  GtkEditable *custom_icon_text_row;
  GtkImage *icon_image;
  AdwLeaflet *leaflet;
  GtkWidget *remove_action_button;
  GtkWidget *remove_custom_icon_button;
  GtkStack *stack;

  BsStreamDeckButton *button;
  GtkWidget *action_preferences;

  gulong action_changed_id;
  gulong custom_icon_changed_id;
  gulong icon_changed_id;
};

static void on_action_row_activated_cb (GtkListBoxRow            *row,
                                        BsStreamDeckButtonEditor *self);

static void on_custom_icon_text_row_text_changed_cb (GtkEditable              *entry,
                                                     GParamSpec               *pspec,
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

  gtk_list_box_append (self->actions_listbox, expander_row);

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
      g_object_set_data (G_OBJECT (row), "plugin-info", (gpointer) plugin_info);
      g_signal_connect (row, "activated", G_CALLBACK (on_action_row_activated_cb), self);

      image = gtk_image_new_from_icon_name (info->icon_name);
      adw_action_row_add_prefix (ADW_ACTION_ROW (row), image);

      adw_expander_row_add_row (ADW_EXPANDER_ROW (expander_row), row);
    }
}

static void
setup_button (BsStreamDeckButtonEditor *self)
{
  BsStreamDeck *stream_deck;
  BsIcon *custom_icon;

  stream_deck = bs_stream_deck_button_get_stream_deck (self->button);
  custom_icon = bs_stream_deck_button_get_custom_icon (self->button);

  gtk_widget_set_sensitive (GTK_WIDGET (self),
                            bs_stream_deck_button_get_position (self->button) != 0 ||
                            bs_page_get_parent (bs_stream_deck_get_active_page (stream_deck)) == NULL);

  gtk_widget_set_visible (self->remove_custom_icon_button, custom_icon != NULL);

  g_signal_handlers_block_by_func (self->custom_icon_text_row,
                                   on_custom_icon_text_row_text_changed_cb,
                                   self);

  if (custom_icon)
    {
      const char *icon_text = bs_icon_get_text (custom_icon);

      if (g_strcmp0 (gtk_editable_get_text (self->custom_icon_text_row), icon_text) != 0)
        gtk_editable_set_text (self->custom_icon_text_row, icon_text ?: "");

      gtk_color_chooser_set_rgba (self->background_color_button,
                                  bs_icon_get_background_color (custom_icon));
    }
  else
    {
      gtk_color_chooser_set_rgba (self->background_color_button,
                                  &(GdkRGBA) { 0.0, 0.0, 0.0, 0.0 });
      gtk_editable_set_text (self->custom_icon_text_row, "");
    }

  g_signal_handlers_unblock_by_func (self->custom_icon_text_row,
                                     on_custom_icon_text_row_text_changed_cb,
                                     self);
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

  gtk_image_set_from_paintable (self->icon_image, GDK_PAINTABLE (icon));
}

static void
maybe_remove_custom_icon (BsStreamDeckButtonEditor *self)
{
  BsIcon *custom_icon = bs_stream_deck_button_get_custom_icon (self->button);
  GdkRGBA transparent = { 0.0, 0.0, 0.0, 0.0 };
  const char *text;

  if (!custom_icon)
    return;

  text = bs_icon_get_text (custom_icon);

  if (gdk_rgba_equal (bs_icon_get_background_color (custom_icon), &transparent) &&
      !bs_icon_get_file (custom_icon) &&
      !bs_icon_get_icon_name (custom_icon) &&
      !bs_icon_get_paintable (custom_icon) &&
      (!text || strlen (text) == 0))
    {
      bs_stream_deck_button_set_custom_icon (self->button, NULL);
    }

}


/*
 * Callbacks
 */

static void
on_action_row_activated_cb (GtkListBoxRow            *row,
                            BsStreamDeckButtonEditor *self)
{
  g_autoptr (BsAction) new_action = NULL;
  g_autoptr (BsIcon) new_custom_icon = NULL;
  g_autoptr (GError) error = NULL;
  PeasPluginInfo *plugin_info;
  BsActionInfo *action_info;
  BsStreamDeck *stream_deck;
  BsPageItem *item;
  BsIcon *custom_icon;
  BsPage *active_page;

  stream_deck = bs_stream_deck_button_get_stream_deck (self->button);
  active_page = bs_stream_deck_get_active_page (stream_deck);
  plugin_info = g_object_get_data (G_OBJECT (row), "plugin-info");
  action_info = g_object_get_data (G_OBJECT (row), "action-info");

  item = bs_page_get_item (active_page, bs_stream_deck_button_get_position (self->button));
  bs_page_item_set_item_type (item, BS_PAGE_ITEM_ACTION);
  bs_page_item_set_factory (item, peas_plugin_info_get_module_name (plugin_info));
  bs_page_item_set_action (item, action_info->id);

  custom_icon = bs_stream_deck_button_get_custom_icon (self->button);
  if (custom_icon)
    bs_page_item_set_custom_icon (item, bs_icon_to_json (custom_icon));

  bs_page_realize (active_page, self->button, &new_custom_icon, &new_action, &error);

  if (error)
    g_warning ("Error realizing action: %s", error->message);

  bs_stream_deck_button_set_action (self->button, new_action);
  bs_stream_deck_button_set_custom_icon (self->button, new_custom_icon);
  update_action_preferences_group (self);

  adw_leaflet_navigate (self->leaflet, ADW_NAVIGATION_DIRECTION_BACK);
}

static void
on_action_factory_added_cb (PeasExtensionSet *extension_set,
                            PeasPluginInfo   *plugin_info,
                            GObject          *extension,
                            gpointer          user_data)
{
  BsStreamDeckButtonEditor *self = BS_STREAM_DECK_BUTTON_EDITOR (user_data);

  add_action_factory (self, BS_ACTION_FACTORY (extension));
}

static void
on_action_factory_removed_cb (PeasExtensionSet *extension_set,
                              PeasPluginInfo   *plugin_info,
                              GObject          *extension,
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
  g_autoptr (BsIcon) icon = NULL;
  GdkRGBA background_color;

  icon = bs_stream_deck_button_get_custom_icon (self->button);
  if (!icon)
    icon = bs_icon_new_empty ();
  else
    g_object_ref (icon);
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (color_button), &background_color);

  bs_icon_set_background_color (icon, &background_color);
  bs_stream_deck_button_set_custom_icon (self->button, icon);
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
      g_autoptr (GError) error = NULL;
      g_autoptr (BsIcon) icon = NULL;
      g_autoptr (GFile) file = NULL;

      file = gtk_file_chooser_get_file (chooser);
      icon = bs_stream_deck_button_get_custom_icon (self->button);

      if (!icon)
        icon = bs_icon_new_empty ();
      else
        g_object_ref (icon);

      bs_icon_set_file (icon, file, &error);
      if (error)
        {
          g_warning ("Error setting custom icon: %s", error->message);
          goto out;
        }

      bs_stream_deck_button_set_custom_icon (self->button, icon);
    }

out:
  g_object_unref (native);
}

static void
on_custom_icon_button_clicked_cb (AdwPreferencesRow        *row,
                                  BsStreamDeckButtonEditor *self)
{
  g_autoptr (GtkFileFilter) filter = NULL;
  GtkFileChooserNative *native;

  gtk_menu_button_popdown (self->custom_icon_menubutton);

  native = gtk_file_chooser_native_new (_("Select icon"),
                                        GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (self))),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        _("_Open"),
                                        _("_Cancel"));
  gtk_native_dialog_set_modal (GTK_NATIVE_DIALOG (native), TRUE);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All supported formats"));
  gtk_file_filter_add_mime_type (filter, "image/*");
  gtk_file_filter_add_mime_type (filter, "video/*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (native), filter);

  g_signal_connect (native, "response", G_CALLBACK (on_file_chooser_native_response_cb), self);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (native));
}

static void
on_custom_icon_text_row_text_changed_cb (GtkEditable              *row,
                                         GParamSpec               *pspec,
                                         BsStreamDeckButtonEditor *self)
{
  g_autoptr (BsIcon) custom_icon = NULL;
  const char *text;

  text = gtk_editable_get_text (row);
  custom_icon = bs_stream_deck_button_get_custom_icon (self->button);

  if (custom_icon)
    g_object_ref (custom_icon);

  if (strlen (text) > 0)
    {
      if (!custom_icon)
        custom_icon = bs_icon_new_empty ();
      else
        g_object_ref (custom_icon);
      bs_icon_set_text (custom_icon, text);
      bs_stream_deck_button_set_custom_icon (self->button, custom_icon);
    }
  else
    {
      if (custom_icon)
        bs_icon_set_text (custom_icon, text);
      maybe_remove_custom_icon (self);
    }
}

static void
on_go_previous_button_clicked_cb (GtkButton                *button,
                                  BsStreamDeckButtonEditor *self)
{
  adw_leaflet_navigate (self->leaflet, ADW_NAVIGATION_DIRECTION_BACK);
}

static void
on_icons_gridview_activate_cb (GtkGridView              *grid_view,
                               unsigned int              position,
                               BsStreamDeckButtonEditor *self)
{
  g_autoptr (GtkStringObject) string_object = NULL;
  g_autoptr (BsIcon) custom_icon = NULL;
  GtkSelectionModel *selection_model;

  selection_model = gtk_grid_view_get_model (grid_view);
  string_object = g_list_model_get_item (G_LIST_MODEL (selection_model), position);

  custom_icon = bs_stream_deck_button_get_custom_icon (self->button);
  if (!custom_icon)
    custom_icon = bs_icon_new_empty ();
  else
    g_object_ref (custom_icon);

  bs_icon_set_file (custom_icon, NULL, NULL);
  bs_icon_set_paintable (custom_icon, NULL);
  bs_icon_set_icon_name (custom_icon, gtk_string_object_get_string (string_object));
  bs_stream_deck_button_set_custom_icon (self->button, custom_icon);
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
on_remove_custom_icon_button_clicked_cb (GtkButton                *button,
                                         BsStreamDeckButtonEditor *self)
{
  bs_stream_deck_button_set_custom_icon (self->button, NULL);
  gtk_color_chooser_set_rgba (self->background_color_button, &(GdkRGBA) { 0.0, 0.0, 0.0, 0.0 });
}

static void
on_select_action_row_activated_cb (GtkListBoxRow            *row,
                                   BsStreamDeckButtonEditor *self)
{
  /* Collapse all expander rows */
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self->actions_listbox));
       child;
       child = gtk_widget_get_next_sibling (child))
    {
      adw_expander_row_set_expanded (ADW_EXPANDER_ROW (child), FALSE);
    }

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
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, actions_listbox);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, background_color_button);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, builtin_icons_stringlist);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, button_preferences_page);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, custom_icon_menubutton);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, custom_icon_text_row);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, icon_image);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, leaflet);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, remove_action_button);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, remove_custom_icon_button);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonEditor, stack);

  gtk_widget_class_bind_template_callback (widget_class, on_background_color_button_color_set_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_custom_icon_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_custom_icon_text_row_text_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_go_previous_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_icons_gridview_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_remove_action_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_remove_custom_icon_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_select_action_row_activated_cb);

  gtk_widget_class_set_css_name (widget_class, "streamdeckbuttoneditor");
}

static void
bs_stream_deck_button_editor_init (BsStreamDeckButtonEditor *self)
{
  PeasExtensionSet *extension_set;
  g_auto (GStrv) icon_names = NULL;
  GApplication *application;
  GtkIconTheme *icon_theme;

  gtk_widget_init_template (GTK_WIDGET (self));

  application = g_application_get_default ();
  extension_set = bs_application_get_action_factory_set (BS_APPLICATION (application));

  peas_extension_set_foreach (extension_set,
                              (PeasExtensionSetForeachFunc) on_action_factory_added_cb,
                              self);

  g_signal_connect (extension_set, "extension-added", G_CALLBACK (on_action_factory_added_cb), self);
  g_signal_connect (extension_set, "extension-removed", G_CALLBACK (on_action_factory_removed_cb), self);

  icon_theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());
  icon_names = gtk_icon_theme_get_icon_names (icon_theme);

  for (size_t i = 0; icon_names && icon_names[i]; i++)
    {
      if (g_str_has_suffix (icon_names[i], "-symbolic"))
        gtk_string_list_append (self->builtin_icons_stringlist, icon_names[i]);
    }
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

  g_clear_signal_handler (&self->action_changed_id, self->button);
  g_clear_signal_handler (&self->custom_icon_changed_id, self->button);
  g_clear_signal_handler (&self->icon_changed_id, self->button);

  if (g_set_object (&self->button, button))
    {
      gtk_stack_set_visible_child_name (self->stack, button ? "button" : "empty");
      update_action_preferences_group (self);
      setup_button (self);
      update_icon (self);

      adw_leaflet_navigate (self->leaflet, ADW_NAVIGATION_DIRECTION_BACK);

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
