/* bs-profiles-dialog.c
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

#include "bs-profile.h"
#include "bs-profile-row.h"
#include "bs-profiles-dialog.h"
#include "bs-stream-deck.h"

struct _BsProfilesDialog
{
  AdwWindow parent_instance;

  GtkListBox *profiles_listbox;

  BsStreamDeck *stream_deck;
};

static void on_profile_row_move_cb (BsProfileRow     *profile_row,
                                    unsigned int      new_position,
                                    BsProfilesDialog *self);

G_DEFINE_FINAL_TYPE (BsProfilesDialog, bs_profiles_dialog, ADW_TYPE_WINDOW)

enum
{
  PROP_0,
  PROP_STREAM_DECK,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/*
 * Callbacks
 */

static void
on_profile_row_move_cb (BsProfileRow     *profile_row,
                        unsigned int      new_position,
                        BsProfilesDialog *self)
{
  GListModel *profiles;
  BsProfile *profile;
  unsigned int position;

  profiles = bs_stream_deck_get_profiles (self->stream_deck);
  profile = bs_profile_row_get_profile (profile_row);

  g_object_ref (profile);
  g_list_store_find (G_LIST_STORE (profiles), profile, &position);
  g_list_store_remove (G_LIST_STORE (profiles), position);
  g_list_store_insert (G_LIST_STORE (profiles), new_position, profile);
  g_object_unref (profile);
}

static GtkWidget *
create_profile_row_cb (gpointer item,
                       gpointer user_data)
{
  BsProfilesDialog *self = BS_PROFILES_DIALOG (user_data);
  BsProfile *profile = BS_PROFILE (item);
  GtkWidget *row;

  row = bs_profile_row_new (self->stream_deck, profile);
  g_signal_connect (row, "move", G_CALLBACK (on_profile_row_move_cb), self);

  return row;
}


/*
 * GObject overrides
 */

static void
bs_profiles_dialog_finalize (GObject *object)
{
  BsProfilesDialog *self = (BsProfilesDialog *)object;

  g_clear_object (&self->stream_deck);

  G_OBJECT_CLASS (bs_profiles_dialog_parent_class)->finalize (object);
}

static void
bs_profiles_dialog_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BsProfilesDialog *self = BS_PROFILES_DIALOG (object);

  switch (prop_id)
    {
    case PROP_STREAM_DECK:
      g_value_set_object (value, self->stream_deck);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_profiles_dialog_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BsProfilesDialog *self = BS_PROFILES_DIALOG (object);

  switch (prop_id)
    {
    case PROP_STREAM_DECK:
      g_assert (self->stream_deck == NULL);
      self->stream_deck = g_value_dup_object (value);

      gtk_list_box_bind_model (self->profiles_listbox,
                               bs_stream_deck_get_profiles (self->stream_deck),
                               create_profile_row_cb,
                               self,
                               NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_profiles_dialog_class_init (BsProfilesDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bs_profiles_dialog_finalize;
  object_class->get_property = bs_profiles_dialog_get_property;
  object_class->set_property = bs_profiles_dialog_set_property;

  properties[PROP_STREAM_DECK] = g_param_spec_object ("stream-deck", NULL, NULL,
                                                      BS_TYPE_STREAM_DECK,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/bs-profiles-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, BsProfilesDialog, profiles_listbox);
}

static void
bs_profiles_dialog_init (BsProfilesDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

BsProfilesDialog *
bs_profiles_dialog_new (BsStreamDeck *stream_deck)
{
  return g_object_new (BS_TYPE_PROFILES_DIALOG,
                       "stream-deck", stream_deck,
                       NULL);
}

BsStreamDeck *
bs_profiles_dialog_get_stream_deck (BsProfilesDialog *self)
{
  g_return_val_if_fail (BS_IS_PROFILES_DIALOG (self), NULL);

  return self->stream_deck;
}
