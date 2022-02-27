/* boatswain-window.c
 *
 * Copyright 2022 Georges Basile Stavracas Neto
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
 */

#include "bs-application.h"
#include "bs-config.h"
#include "bs-device-manager.h"
#include "bs-profile.h"
#include "bs-profiles-dialog.h"
#include "bs-stream-deck.h"
#include "bs-stream-deck-editor.h"
#include "bs-window.h"

#include <adwaita.h>

struct _BsWindow
{
  GtkApplicationWindow  parent_instance;

  GtkWidget *create_profile_button;
  GtkHeaderBar *header_bar;
  GtkWidget *empty_page;
  GtkStack *main_stack;
  GtkListBox *profiles_listbox;
  GtkEditable *new_profile_name_entry;
  GtkMenuButton *profiles_menu_button;
  GtkImage *stream_deck_icon;
  GtkLabel *stream_deck_name_label;
  GtkMenuButton *stream_deck_menubutton;
  GtkListBox *stream_decks_listbox;

  BsStreamDeck *current_stream_deck;
  GBinding *profile_name_binding;
  GtkWindow *profiles_dialog;
  gulong active_profile_changed_id;
};

static GtkWidget * create_profile_row_cb (gpointer item,
                                          gpointer user_data);

static void on_current_stream_deck_active_profile_changed_cb (BsStreamDeck *stream_deck,
                                                              GParamSpec   *pspec,
                                                              BsWindow     *self);

G_DEFINE_FINAL_TYPE (BsWindow, bs_window, GTK_TYPE_APPLICATION_WINDOW)


/*
 * Auxiliary methods
 */

static void
append_new_profile (BsWindow *self)
{
  g_autoptr (BsProfile) new_profile = NULL;
  g_autofree char *new_profile_name = NULL;
  GListModel *profiles;

  if (!gtk_widget_get_sensitive (self->create_profile_button))
    return;

  new_profile_name = g_strdup (gtk_editable_get_text (self->new_profile_name_entry));
  g_strstrip (new_profile_name);

  new_profile = bs_profile_new_empty ();
  bs_profile_set_name (new_profile, new_profile_name);

  profiles = bs_stream_deck_get_profiles (self->current_stream_deck);
  g_list_store_append (G_LIST_STORE (profiles), new_profile);

  bs_stream_deck_load_profile (self->current_stream_deck, new_profile);

  gtk_editable_set_text (self->new_profile_name_entry, "");
  gtk_menu_button_popdown (self->profiles_menu_button);
}

static void
update_active_profile (BsWindow *self)
{
  GListModel *profiles;
  BsProfile *active_profile;
  unsigned int i;

  active_profile = bs_stream_deck_get_active_profile (self->current_stream_deck);

  g_clear_pointer (&self->profile_name_binding, g_binding_unbind);
  self->profile_name_binding = g_object_bind_property (active_profile,
                                                       "name",
                                                       self->profiles_menu_button,
                                                       "label",
                                                       G_BINDING_SYNC_CREATE);

  profiles = bs_stream_deck_get_profiles (self->current_stream_deck);
  for (i = 0; i < g_list_model_get_n_items (profiles); i++)
    {
      g_autoptr (BsProfile) profile = NULL;
      GtkListBoxRow *row;
      GtkWidget *image;

      profile = g_list_model_get_item (profiles, i);
      row = gtk_list_box_get_row_at_index (self->profiles_listbox, i);
      image = g_object_get_data (G_OBJECT (row), "selected-icon");

      gtk_widget_set_child_visible (image, profile == active_profile);
    }
}

static void
validate_rename_entry (BsWindow *self)
{
  g_autofree char *new_name = NULL;
  gboolean valid;

  new_name = g_strdup (gtk_editable_get_text (self->new_profile_name_entry));
  valid = new_name != NULL && g_utf8_strlen (g_strstrip (new_name), -1) > 0;

  gtk_widget_set_sensitive (self->create_profile_button, valid);
}

static void
select_stream_deck (BsWindow     *self,
                    BsStreamDeck *stream_deck)
{
  g_autofree char *page_name = NULL;

  if (self->current_stream_deck == stream_deck)
    return;

  page_name = g_strdup_printf ("%p", stream_deck);
  gtk_stack_set_visible_child_name (self->main_stack, page_name);

  gtk_widget_set_sensitive (GTK_WIDGET (self->stream_deck_menubutton), stream_deck != NULL);
  gtk_image_set_from_gicon (self->stream_deck_icon, bs_stream_deck_get_icon (stream_deck));
  gtk_label_set_label (self->stream_deck_name_label, bs_stream_deck_get_name (stream_deck));

  g_clear_signal_handler (&self->active_profile_changed_id, self->current_stream_deck);

  self->current_stream_deck = stream_deck;

  if (stream_deck)
    {
      self->active_profile_changed_id = g_signal_connect (stream_deck,
                                                          "notify::active-profile",
                                                          G_CALLBACK (on_current_stream_deck_active_profile_changed_cb),
                                                          self);
    }

  gtk_list_box_bind_model (self->profiles_listbox,
                           stream_deck ? bs_stream_deck_get_profiles (stream_deck) : NULL,
                           create_profile_row_cb,
                           self,
                           NULL);

  update_active_profile (self);
}


/*
 * Callbacks
 */

static GtkWidget *
create_profile_row_cb (gpointer item,
                       gpointer user_data)
{
  BsProfile *profile;
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *box;
  GtkWidget *row;
  BsWindow *self;

  self = BS_WINDOW (user_data);
  profile = BS_PROFILE (item);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  g_object_bind_property (profile, "name", label, "label", G_BINDING_SYNC_CREATE);
  gtk_box_append (GTK_BOX (box), label);

  image = gtk_image_new_from_icon_name ("object-select-symbolic");
  gtk_widget_set_child_visible (image, profile == bs_stream_deck_get_active_profile (self->current_stream_deck));
  gtk_box_append (GTK_BOX (box), image);

  row = gtk_list_box_row_new ();
  gtk_list_box_row_set_child (GTK_LIST_BOX_ROW (row), box);
  g_object_set_data (G_OBJECT (row), "selected-icon", image);

  return row;
}

static GtkWidget *
create_stream_deck_row_cb (gpointer item,
                           gpointer user_data)
{
  BsStreamDeck *stream_deck;
  GtkWidget *label;
  GtkWidget *icon;
  GtkWidget *box;
  GtkWidget *row;

  stream_deck = BS_STREAM_DECK (item);
  label = gtk_label_new (bs_stream_deck_get_name (stream_deck));
  icon = gtk_image_new_from_gicon (bs_stream_deck_get_icon (stream_deck));

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_append (GTK_BOX (box), icon);
  gtk_box_append (GTK_BOX (box), label);

  row = gtk_list_box_row_new ();
  gtk_list_box_row_set_child (GTK_LIST_BOX_ROW (row), box);
  g_object_set_data (G_OBJECT (row), "stream-deck", item);

  return row;
}

static void
on_create_profile_button_clicked_cb (GtkButton *button,
                                     BsWindow  *self)
{
  append_new_profile (self);
}

static void
on_current_stream_deck_active_profile_changed_cb (BsStreamDeck *stream_deck,
                                                  GParamSpec   *pspec,
                                                  BsWindow     *self)
{
  update_active_profile (self);
}

static void
on_device_manager_stream_deck_added_cb (BsDeviceManager *device_manager,
                                        BsStreamDeck    *stream_deck,
                                        BsWindow        *self)
{
  g_autofree char *page_name = NULL;
  GListModel *stream_decks;
  GtkWidget *editor;

  editor = bs_stream_deck_editor_new (stream_deck);
  page_name = g_strdup_printf ("%p", stream_deck);
  gtk_stack_add_named (self->main_stack, editor, page_name);

  stream_decks = bs_device_manager_get_stream_decks (device_manager);
  if (g_list_model_get_n_items (stream_decks) == 1)
    select_stream_deck (self, stream_deck);

  gtk_widget_show (GTK_WIDGET (self->profiles_menu_button));
}

static void
on_device_manager_stream_deck_removed_cb (BsDeviceManager *device_manager,
                                          BsStreamDeck    *stream_deck,
                                          BsWindow        *self)
{
  g_autofree char *page_name = NULL;
  GListModel *stream_decks;
  GtkWidget *child;
  unsigned int n_stream_decks;

  page_name = g_strdup_printf ("%p", stream_deck);
  child = gtk_stack_get_child_by_name (self->main_stack, page_name);

  gtk_stack_remove (self->main_stack, child);

  stream_decks = bs_device_manager_get_stream_decks (device_manager);
  n_stream_decks = g_list_model_get_n_items (stream_decks);
  gtk_widget_set_visible (GTK_WIDGET (self->profiles_menu_button), n_stream_decks > 0);

  if (n_stream_decks == 0)
    gtk_stack_set_visible_child (self->main_stack, self->empty_page);
}

static void
on_manage_profiles_action_activated_cb (GSimpleAction *simple,
                                        GVariant      *parameter,
                                        gpointer       user_data)
{
  BsWindow *self = BS_WINDOW (user_data);

  g_clear_pointer (&self->profiles_dialog, gtk_window_destroy);

  if (!self->current_stream_deck)
    return;

  self->profiles_dialog = GTK_WINDOW (bs_profiles_dialog_new (self->current_stream_deck));
  gtk_window_set_transient_for (self->profiles_dialog, GTK_WINDOW (self));
  gtk_window_present (self->profiles_dialog);

  g_object_add_weak_pointer (G_OBJECT (self->profiles_dialog),
                             (gpointer *) &self->profiles_dialog);
}

static void
on_new_profile_name_entry_activate_cb (GtkEntry *entry,
                                       BsWindow *self)
{
  append_new_profile (self);
}

static void
on_new_profile_name_entry_text_changed_cb (GtkEntry   *entry,
                                           GParamSpec *pspec,
                                           BsWindow   *self)
{
  validate_rename_entry (self);
}

static void
on_profiles_listbox_row_activated_cb (GtkListBox    *listbox,
                                      GtkListBoxRow *row,
                                      BsWindow      *self)
{
  g_autoptr (BsProfile) profile = NULL;
  GListModel *profiles;

  profiles = bs_stream_deck_get_profiles (self->current_stream_deck);
  profile = g_list_model_get_item (profiles, gtk_list_box_row_get_index (row));

  bs_stream_deck_load_profile (self->current_stream_deck, profile);

  update_active_profile (self);
  gtk_menu_button_popdown (self->profiles_menu_button);
}

static void
on_stream_decks_listbox_row_activated_cb (GtkListBox    *listbox,
                                          GtkListBoxRow *row,
                                          BsWindow      *self)
{
  BsStreamDeck *stream_deck;

  stream_deck = g_object_get_data (G_OBJECT (row), "stream-deck");
  select_stream_deck (self, stream_deck);

  gtk_menu_button_popdown (self->stream_deck_menubutton);
}


/*
 * GObject overrides
 */

static void
bs_window_finalize (GObject *object)
{
  BsWindow *self = (BsWindow *)object;

  g_clear_signal_handler (&self->active_profile_changed_id, self->current_stream_deck);

  G_OBJECT_CLASS (bs_window_parent_class)->finalize (object);
}

static void
bs_window_constructed (GObject *object)
{
  BsDeviceManager *device_manager;
  GApplication *application;
  GListModel *stream_decks;
  BsWindow *self;
  gboolean first;
  size_t i;

  G_OBJECT_CLASS (bs_window_parent_class)->constructed (object);

  self = BS_WINDOW (object);
  first = TRUE;
  application = g_application_get_default ();
  device_manager = bs_application_get_device_manager (BS_APPLICATION (application));
  stream_decks = bs_device_manager_get_stream_decks (device_manager);

  for (i = 0; i < g_list_model_get_n_items (stream_decks); i++)
    {
      g_autoptr (BsStreamDeck) stream_deck = NULL;
      g_autofree char *page_name = NULL;
      GtkWidget *editor;

      stream_deck = g_list_model_get_item (stream_decks, i);
      editor = bs_stream_deck_editor_new (stream_deck);
      page_name = g_strdup_printf ("%p", stream_deck);
      gtk_stack_add_named (self->main_stack, editor, page_name);

      if (first)
        select_stream_deck (self, stream_deck);

      gtk_widget_show (GTK_WIDGET (self->profiles_menu_button));
    }

  gtk_list_box_bind_model (self->stream_decks_listbox,
                           stream_decks,
                           create_stream_deck_row_cb,
                           self,
                           NULL);

  g_signal_connect_object (device_manager,
                           "stream-deck-added",
                           G_CALLBACK (on_device_manager_stream_deck_added_cb),
                           self,
                           0);

  g_signal_connect_object (device_manager,
                           "stream-deck-removed",
                           G_CALLBACK (on_device_manager_stream_deck_removed_cb),
                           self,
                           0);
}

static void
bs_window_class_init (BsWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bs_window_finalize;
  object_class->constructed = bs_window_constructed;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/bs-window.ui");

  gtk_widget_class_bind_template_child (widget_class, BsWindow, create_profile_button);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, empty_page);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, main_stack);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, new_profile_name_entry);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, profiles_listbox);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, profiles_menu_button);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, stream_deck_icon);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, stream_deck_name_label);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, stream_deck_menubutton);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, stream_decks_listbox);

  gtk_widget_class_bind_template_callback (widget_class, on_create_profile_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_new_profile_name_entry_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_new_profile_name_entry_text_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_profiles_listbox_row_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_stream_decks_listbox_row_activated_cb);
}

static void
bs_window_init (BsWindow *self)
{
  const GActionEntry actions[] = {
    { "manage-profiles", on_manage_profiles_action_activated_cb, },
  };

  gtk_widget_init_template (GTK_WIDGET (self));

  g_action_map_add_action_entries (G_ACTION_MAP (self), actions, G_N_ELEMENTS (actions), self);
}
