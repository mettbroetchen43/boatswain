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
#include "bs-stream-deck.h"
#include "bs-stream-deck-editor.h"
#include "bs-window.h"

#include <adwaita.h>

struct _BsWindow
{
  GtkApplicationWindow  parent_instance;

  GtkHeaderBar *header_bar;
  GtkWidget *empty_page;
  GtkStack *main_stack;
  GtkImage *stream_deck_icon;
  GtkLabel *stream_deck_name_label;
  GtkMenuButton *stream_deck_menubutton;
  GtkListBox *stream_decks_listbox;

  BsStreamDeck *current_stream_deck;
};

G_DEFINE_TYPE (BsWindow, bs_window, GTK_TYPE_APPLICATION_WINDOW)


/*
 * Auxiliary methods
 */

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

  self->current_stream_deck = stream_deck;
}


/*
 * Callbacks
 */

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
}

static void
on_device_manager_stream_deck_removed_cb (BsDeviceManager *device_manager,
                                          BsStreamDeck    *stream_deck,
                                          BsWindow        *self)
{
  g_autofree char *page_name = NULL;
  GListModel *stream_decks;
  GtkWidget *child;

  page_name = g_strdup_printf ("%p", stream_deck);
  child = gtk_stack_get_child_by_name (self->main_stack, page_name);

  gtk_stack_remove (self->main_stack, child);

  stream_decks = bs_device_manager_get_stream_decks (device_manager);
  if (g_list_model_get_n_items (stream_decks) == 0)
    gtk_stack_set_visible_child (self->main_stack, self->empty_page);
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

  object_class->constructed = bs_window_constructed;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/bs-window.ui");

  gtk_widget_class_bind_template_child (widget_class, BsWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, empty_page);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, main_stack);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, stream_deck_icon);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, stream_deck_name_label);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, stream_deck_menubutton);
  gtk_widget_class_bind_template_child (widget_class, BsWindow, stream_decks_listbox);

  gtk_widget_class_bind_template_callback (widget_class, on_stream_decks_listbox_row_activated_cb);
}

static void
bs_window_init (BsWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
