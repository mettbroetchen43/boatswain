/* bs-stream-deck-button.c
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
#include "bs-page.h"
#include "bs-stream-deck-private.h"
#include "bs-stream-deck-button-private.h"

struct _BsStreamDeckButton
{
  GObject parent_instance;

  BsAction *action;
  BsIcon *custom_icon;
  BsStreamDeck *stream_deck;
  uint8_t position;
  gulong custom_contents_changed_id;
  gulong custom_size_changed_id;
  gulong action_contents_changed_id;
  gulong action_size_changed_id;
  gboolean pressed;
};

G_DEFINE_FINAL_TYPE (BsStreamDeckButton, bs_stream_deck_button, G_TYPE_OBJECT)

enum
{
  PROP_0,
  PROP_ACTION,
  PROP_ICON,
  PROP_CUSTOM_ICON,
  PROP_PRESSED,
  N_PROPS,
};

enum
{
  ICON_CHANGED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];
static GParamSpec *properties[N_PROPS];


/*
 * Auxiliary methods
 */

static void
update_page (BsStreamDeckButton *self)
{
  BsPage *page = bs_stream_deck_get_active_page (self->stream_deck);

  if (page)
    bs_page_update_button (page, self);
}

static void
update_icon (BsStreamDeckButton *self)
{
  g_autoptr (GError) error = NULL;
  BsIcon *icon;

  icon = bs_stream_deck_button_get_icon (self);

  bs_stream_deck_set_button_icon (self->stream_deck,
                                  self->position,
                                  icon,
                                  &error);

  if (error)
    g_warning ("Error updating Stream Deck button icon: %s", error->message);
}

static void
remove_custom_icon (BsStreamDeckButton *self)
{
  if (self->custom_icon)
    {
      g_clear_signal_handler (&self->custom_contents_changed_id, self->custom_icon);
      g_clear_signal_handler (&self->custom_size_changed_id, self->custom_icon);
      g_clear_object (&self->custom_icon);
    }
}


/*
 * Callbacks
 */

static void
on_icon_changed_cb (BsIcon             *icon,
                    BsStreamDeckButton *self)
{
  update_icon (self);
  g_signal_emit (self, signals[ICON_CHANGED], 0, icon);
}


/*
 * GObject overrides
 */

static void
bs_stream_deck_button_finalize (GObject *object)
{
  BsStreamDeckButton *self = (BsStreamDeckButton *)object;

  remove_custom_icon (self);

  if (self->action)
    {
      g_clear_signal_handler (&self->action_contents_changed_id, bs_action_get_icon (self->action));
      g_clear_signal_handler (&self->action_size_changed_id, bs_action_get_icon (self->action));
      g_clear_object (&self->action);
    }

  G_OBJECT_CLASS (bs_stream_deck_button_parent_class)->finalize (object);
}

static void
bs_stream_deck_button_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BsStreamDeckButton *self = BS_STREAM_DECK_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ACTION:
      g_value_set_object (value, self->action);
      break;

    case PROP_ICON:
      g_value_set_object (value, bs_stream_deck_button_get_icon (self));
      break;

    case PROP_CUSTOM_ICON:
      g_value_set_object (value, self->custom_icon);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_button_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BsStreamDeckButton *self = BS_STREAM_DECK_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ACTION:
      bs_stream_deck_button_set_action (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_button_class_init (BsStreamDeckButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_stream_deck_button_finalize;
  object_class->get_property = bs_stream_deck_button_get_property;
  object_class->set_property = bs_stream_deck_button_set_property;

  properties[PROP_ACTION] = g_param_spec_object ("action", NULL, NULL,
                                                 BS_TYPE_ACTION,
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_ICON] = g_param_spec_object ("icon", NULL, NULL,
                                               BS_TYPE_ICON,
                                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_CUSTOM_ICON] = g_param_spec_object ("custom-icon", NULL, NULL,
                                                      BS_TYPE_ICON,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_PRESSED] = g_param_spec_boolean ("pressed", NULL, NULL,
                                                   FALSE,
                                                   G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals[ICON_CHANGED] = g_signal_new ("icon-changed",
                                        BS_TYPE_STREAM_DECK_BUTTON,
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL, NULL,
                                        G_TYPE_NONE,
                                        1,
                                        BS_TYPE_ICON);
}

static void
bs_stream_deck_button_init (BsStreamDeckButton *self)
{
  self->pressed = FALSE;
}

BsStreamDeckButton *
bs_stream_deck_button_new (BsStreamDeck *stream_deck,
                           uint8_t       position)
{
  g_autoptr (BsIcon) empty_icon = NULL;
  BsStreamDeckButton *self;

  self = g_object_new (BS_TYPE_STREAM_DECK_BUTTON, NULL);
  self->stream_deck = stream_deck;
  self->position = position;

  empty_icon = bs_icon_new_empty ();
  bs_stream_deck_button_set_custom_icon (self, empty_icon, NULL);

  return self;
}

BsStreamDeck *
bs_stream_deck_button_get_stream_deck (BsStreamDeckButton *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK_BUTTON (self), NULL);

  return self->stream_deck;
}

uint8_t
bs_stream_deck_button_get_position (BsStreamDeckButton *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK_BUTTON (self), 0);

  return self->position;
}

gboolean
bs_stream_deck_button_get_pressed (BsStreamDeckButton *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK_BUTTON (self), FALSE);

  return self->pressed;
}

void
bs_stream_deck_button_set_pressed (BsStreamDeckButton *self,
                                   gboolean            pressed)
{
  g_return_if_fail (BS_IS_STREAM_DECK_BUTTON (self));

  if (self->pressed == pressed)
    return;

  self->pressed = pressed;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRESSED]);

  if (self->action)
    {
      if (pressed)
        bs_action_activate (self->action);
      else
        bs_action_deactivate (self->action);
    }
}

BsIcon *
bs_stream_deck_button_get_icon (BsStreamDeckButton *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK_BUTTON (self), NULL);

  if (self->custom_icon)
    return self->custom_icon;

  if (self->action)
    return bs_action_get_icon (self->action);

  return NULL;
}

BsIcon *
bs_stream_deck_button_get_custom_icon (BsStreamDeckButton *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK_BUTTON (self), NULL);

  return self->custom_icon;
}

gboolean
bs_stream_deck_button_set_custom_icon (BsStreamDeckButton  *self,
                                       BsIcon              *icon,
                                       GError             **error)
{
  gboolean result;

  g_return_val_if_fail (BS_IS_STREAM_DECK_BUTTON (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (self->custom_icon == icon)
    return TRUE;

  result = bs_stream_deck_set_button_icon (self->stream_deck,
                                           self->position,
                                           icon,
                                           error);

  if (!result)
    return FALSE;

  remove_custom_icon (self);

  g_set_object (&self->custom_icon, icon);

  if (icon)
    {
      self->custom_contents_changed_id =
        g_signal_connect (icon, "invalidate-contents", G_CALLBACK (on_icon_changed_cb), self);

      self->custom_size_changed_id =
        g_signal_connect (icon, "invalidate-size", G_CALLBACK (on_icon_changed_cb), self);
    }

  update_page (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CUSTOM_ICON]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
  g_signal_emit (self, signals[ICON_CHANGED], 0, icon);

  return TRUE;
}


BsAction *
bs_stream_deck_button_get_action (BsStreamDeckButton *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK_BUTTON (self), NULL);

  return self->action;
}

void
bs_stream_deck_button_set_action (BsStreamDeckButton *self,
                                  BsAction           *action)
{
  BsIcon *action_icon;

  g_return_if_fail (BS_IS_STREAM_DECK_BUTTON (self));

  if (self->action == action)
    return;

  if (action)
    remove_custom_icon (self);

  if (self->action)
    {
      g_clear_signal_handler (&self->action_contents_changed_id, bs_action_get_icon (self->action));
      g_clear_signal_handler (&self->action_size_changed_id, bs_action_get_icon (self->action));
    }

  g_set_object (&self->action, action);

  action_icon = bs_action_get_icon (action);
  self->action_contents_changed_id =
    g_signal_connect (action_icon, "invalidate-contents", G_CALLBACK (on_icon_changed_cb), self);

  self->action_size_changed_id =
    g_signal_connect (action_icon, "invalidate-size", G_CALLBACK (on_icon_changed_cb), self);

  update_icon (self);
  update_page (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTION]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CUSTOM_ICON]);

  g_signal_emit (self, signals[ICON_CHANGED], 0, bs_stream_deck_button_get_icon (self));
}
