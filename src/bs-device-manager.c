/* bs-device-manager.c
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

#include <gusb.h>

#include "bs-device-enums.h"
#include "bs-device-manager.h"
#include "bs-stream-deck.h"

struct _BsDeviceManager
{
  GObject parent_instance;

  GUsbContext *gusb_context;
  GListStore *stream_decks;
};

G_DEFINE_FINAL_TYPE (BsDeviceManager, bs_device_manager, G_TYPE_OBJECT)

enum
{
  STREAM_DECK_ADDED,
  STREAM_DECK_REMOVED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS] = { 0, };

/*
 * Auxiliary methods
 */

static void
enumerate_stream_decks (BsDeviceManager *self)
{
  g_autoptr (GPtrArray) devices = NULL;
  unsigned int i;

  g_usb_context_enumerate (self->gusb_context);

  devices = g_usb_context_get_devices (self->gusb_context);
  for (i = 0; devices && i < devices->len; i++)
    {
      g_autoptr (BsStreamDeck) stream_deck = NULL;
      g_autoptr (GError) error = NULL;
      GUsbDevice *usb_device;

      usb_device = g_ptr_array_index (devices, i);
      stream_deck = bs_stream_deck_new (usb_device, &error);

      if (error)
        {
          if (!g_error_matches (error, BS_STREAM_DECK_ERROR, BS_STREAM_DECK_ERROR_UNRECONIZED))
            g_warning ("Error opening Stream Deck device: %s", error->message);
          continue;
        }

      g_message ("Found %s", bs_stream_deck_get_name (stream_deck));

      g_list_store_append (self->stream_decks, stream_deck);
    }
}


/*
 * Callbacks
 */

static void
on_gusb_context_device_added_cb (GUsbContext     *gusb_context,
                                 GUsbDevice      *device,
                                 BsDeviceManager *self)
{
  g_autoptr (BsStreamDeck) stream_deck = NULL;
  g_autoptr (GError) error = NULL;

  stream_deck = bs_stream_deck_new (device, &error);

  if (error)
    {
      if (!g_error_matches (error, BS_STREAM_DECK_ERROR, BS_STREAM_DECK_ERROR_UNRECONIZED))
        g_warning ("Error opening Stream Deck device: %s", error->message);
      return;
    }

  g_message ("Adding %s", bs_stream_deck_get_name (stream_deck));

  g_list_store_append (self->stream_decks, g_object_ref (stream_deck));
  g_signal_emit (self, signals[STREAM_DECK_ADDED], 0, stream_deck);
}

static void
on_gusb_context_device_removed_cb (GUsbContext     *gusb_context,
                                   GUsbDevice      *device,
                                   BsDeviceManager *self)
{
  unsigned int i = 0;

  while (i < g_list_model_get_n_items (G_LIST_MODEL (self->stream_decks)))
    {
      g_autoptr (BsStreamDeck) stream_deck = NULL;
      GUsbDevice *d;

      stream_deck = g_list_model_get_item (G_LIST_MODEL (self->stream_decks), i);
      d = bs_stream_deck_get_device (stream_deck);

      if (d == device)
        {
          g_message ("Removing Stream Deck device %p", stream_deck);
          g_signal_emit (self, signals[STREAM_DECK_REMOVED], 0, stream_deck);
          g_list_store_remove (self->stream_decks, i);
          continue;
        }

      i++;
    }
}


/*
 * GObject overrides
 */

static void
bs_device_manager_finalize (GObject *object)
{
  BsDeviceManager *self = (BsDeviceManager *)object;

  g_clear_object (&self->stream_decks);
  g_clear_object (&self->gusb_context);

  G_OBJECT_CLASS (bs_device_manager_parent_class)->finalize (object);
}

static void
bs_device_manager_class_init (BsDeviceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_device_manager_finalize;

  signals[STREAM_DECK_ADDED] = g_signal_new ("stream-deck-added",
                                             BS_TYPE_DEVICE_MANAGER,
                                             G_SIGNAL_RUN_LAST,
                                             0, NULL, NULL, NULL,
                                             G_TYPE_NONE,
                                             1,
                                             BS_TYPE_STREAM_DECK);

  signals[STREAM_DECK_REMOVED] = g_signal_new ("stream-deck-removed",
                                               BS_TYPE_DEVICE_MANAGER,
                                               G_SIGNAL_RUN_LAST,
                                               0, NULL, NULL, NULL,
                                               G_TYPE_NONE,
                                               1,
                                               BS_TYPE_STREAM_DECK);
}

static void
bs_device_manager_init (BsDeviceManager *self)
{
  g_autoptr (GError) error = NULL;

  self->stream_decks = g_list_store_new (BS_TYPE_STREAM_DECK);

  self->gusb_context = g_usb_context_new (&error);
  enumerate_stream_decks (self);
  g_signal_connect (self->gusb_context, "device-added", G_CALLBACK (on_gusb_context_device_added_cb), self);
  g_signal_connect (self->gusb_context, "device-removed", G_CALLBACK (on_gusb_context_device_removed_cb), self);
}

BsDeviceManager *
bs_device_manager_new (void)
{
  return g_object_new (BS_TYPE_DEVICE_MANAGER, NULL);
}

GListModel *
bs_device_manager_get_stream_decks (BsDeviceManager *self)
{
  g_return_val_if_fail (BS_IS_DEVICE_MANAGER (self), NULL);

  return G_LIST_MODEL (self->stream_decks);
}
