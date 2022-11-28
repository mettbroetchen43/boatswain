/* bs-stream-deck.c
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

#define G_LOG_DOMAIN "Stream Deck"

#include "bs-action.h"
#include "bs-debug.h"
#include "bs-device-enums.h"
#include "bs-icon.h"
#include "bs-icon-renderer.h"
#include "bs-page.h"
#include "bs-profile.h"
#include "bs-stream-deck-private.h"
#include "bs-stream-deck-button-private.h"

#include <glib/gi18n.h>
#include <hidapi.h>

#define POLL_RATE_MS 16

typedef struct {
  uint8_t product_id;
  const char *name;
  const char *icon_name;
  BsStreamDeckButtonLayout button_layout;
  BsIconLayout icon_layout;

  void (*reset) (BsStreamDeck *self);
  void (*set_brightness) (BsStreamDeck *self,
                          double        brightness);

  char * (*get_serial_number) (BsStreamDeck *self);
  char * (*get_firmware_version) (BsStreamDeck *self);
  gboolean (*set_button_texture) (BsStreamDeck  *self,
                                  uint8_t        button,
                                  GdkTexture    *texture,
                                  GError       **error);
  gboolean (*read_button_states) (BsStreamDeck *self);
  guint poll_timeout_id;
} StreamDeckModelInfo;

typedef struct
{
  GSource source;
  BsStreamDeck *stream_deck;
} StreamDeckSource;

struct _BsStreamDeck
{
  GObject parent_instance;

  BsIconRenderer *icon_renderer;
  GListStore *profiles;
  BsProfile *active_profile;
  GQueue *active_pages;
  guint save_timeout_id;

  const StreamDeckModelInfo *model_info;
  GUsbDevice *device;
  hid_device *handle;

  double brightness;
  char *serial_number;
  char *firmware_version;
  GIcon *icon;
  GPtrArray *buttons;
  GSource *poll_source;
  gboolean initialized;
  gboolean loaded;
  gboolean fake;
};

static void g_initable_iface_init (GInitableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BsStreamDeck, bs_stream_deck, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, g_initable_iface_init))

G_DEFINE_QUARK (BsStreamDeck, bs_stream_deck_error);

enum
{
  PROP_0,
  PROP_ACTIVE_PAGE,
  PROP_ACTIVE_PROFILE,
  PROP_BRIGHTNESS,
  PROP_DEVICE,
  PROP_FAKE,
  PROP_ICON,
  PROP_NAME,
  N_PROPS,
};

enum
{
  BUTTON_PRESSED,
  BUTTON_RELEASED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];
static GParamSpec *properties[N_PROPS];


/*
 * Auxiliary methods
 */

static char *
get_profile_path (BsStreamDeck *self)
{
  g_autofree char *profile_filename = NULL;

  profile_filename = g_strdup_printf ("%s.json", self->serial_number);

  return g_build_filename (g_get_user_data_dir (),
                           profile_filename,
                           NULL);
}

static void
update_pages (BsStreamDeck *self)
{
  BsPage *active_page;
  GList *l;
  unsigned int i;

  BS_ENTRY;

  active_page = bs_stream_deck_get_active_page (self);

  for (i = 0; i < self->model_info->button_layout.n_buttons; i++)
    {
      BsStreamDeckButton *stream_deck_button = g_ptr_array_index (self->buttons, i);
      bs_page_update_item_from_button (active_page, stream_deck_button);
    }

  for (l = g_queue_peek_head_link (self->active_pages); l; l = l->next)
    bs_page_update_all_items (l->data);

  BS_EXIT;
}

static void
save_profiles (BsStreamDeck *self)
{
  g_autoptr (JsonGenerator) generator = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) root = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree char *profile_path = NULL;
  g_autofree char *json_str = NULL;
  unsigned int i;

  BS_ENTRY;

  if (self->fake)
    BS_RETURN ();

  /* Update the active profile */
  bs_profile_set_brightness (self->active_profile, self->brightness);
  update_pages (self);

  builder = json_builder_new ();

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "active-profile");
  json_builder_add_string_value (builder, bs_profile_get_id (self->active_profile));

  json_builder_set_member_name (builder, "profiles");
  json_builder_begin_array (builder);
  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->profiles)); i++)
    {
      g_autoptr (BsProfile) profile = NULL;

      profile = g_list_model_get_item (G_LIST_MODEL (self->profiles), i);
      json_builder_add_value (builder, bs_profile_to_json (profile));
    }
  json_builder_end_array (builder);

  json_builder_end_object (builder);

  root = json_builder_get_root (builder);

  generator = json_generator_new ();
  json_generator_set_pretty (generator, TRUE);
  json_generator_set_root (generator, root);
  json_str = json_generator_to_data (generator, NULL);

  profile_path = get_profile_path (self);
  g_file_set_contents (profile_path, json_str, -1, &error);

  if (error)
    g_warning ("Error saving profiles: %s", error->message);

  BS_EXIT;
}

static void
load_profiles (BsStreamDeck  *self)
{
  g_autoptr (JsonParser) parser = NULL;
  g_autoptr (BsProfile) active_profile = NULL;
  g_autoptr (GError) local_error = NULL;
  g_autofree char *profile_path = NULL;
  JsonObject *object;
  JsonArray *profiles_array;
  JsonNode *root;
  unsigned int i;
  const char *active_profile_id;

  BS_ENTRY;

  profile_path = get_profile_path (self);

  g_debug ("Loading %s", profile_path);

  parser = json_parser_new ();

  json_parser_load_from_file (parser, profile_path, &local_error);
  if (local_error)
    {
      g_debug ("Error loading profile for device %s: %s",
               self->serial_number,
               local_error->message);
      BS_GOTO (out);
    }

  root = json_parser_get_root (parser);
  object = json_node_get_object (root);

  active_profile = NULL;
  active_profile_id = json_object_get_string_member (object, "active-profile");

  profiles_array = json_object_get_array_member (object, "profiles");
  for (i = 0; i < json_array_get_length (profiles_array); i++)
    {
      g_autoptr (BsProfile) profile = NULL;
      JsonNode *profile_node;

      profile_node = json_array_get_element (profiles_array, i);

      if (!profile_node)
        continue;

      profile = bs_profile_new_from_json (self, profile_node);
      g_list_store_append (self->profiles, profile);

      if (g_strcmp0 (active_profile_id, bs_profile_get_id (profile)) == 0)
        active_profile = g_object_ref (profile);
    }

  if (!active_profile)
    active_profile = g_list_model_get_item (G_LIST_MODEL (self->profiles), 0);

out:
  if (!active_profile)
    {
      active_profile = bs_profile_new_empty (self);
      bs_profile_set_name (active_profile, _("Default"));
      g_list_store_append (self->profiles, active_profile);
    }

  bs_stream_deck_load_profile (self, active_profile);

  BS_EXIT;
}

static void
load_active_page (BsStreamDeck *self)
{
  BsPage *active_page;
  uint8_t i;

  BS_ENTRY;

  active_page = bs_stream_deck_get_active_page (self);

  for (i = 0; i < self->model_info->button_layout.n_buttons; i++)
    {
      BsStreamDeckButton *stream_deck_button;
      g_autoptr (BsAction) action = NULL;
      g_autoptr (BsIcon) custom_icon = NULL;
      g_autoptr (GError) error = NULL;

      stream_deck_button = g_ptr_array_index (self->buttons, i);

      bs_page_realize (active_page, stream_deck_button, &custom_icon, &action, &error);

      if (error)
        {
          g_warning ("Failed to construct action and icon from page: %s", error->message);
          continue;
        }

      bs_stream_deck_button_inhibit_page_updates (stream_deck_button);

      bs_stream_deck_button_set_action (stream_deck_button, action);
      bs_stream_deck_button_set_custom_icon (stream_deck_button, custom_icon);

      bs_stream_deck_button_uninhibit_page_updates (stream_deck_button);
    }

  BS_EXIT;
}

static inline uint8_t
swap_button_index_original (BsStreamDeck *self,
                            uint8_t       button_index)
{
  int column = button_index % self->model_info->button_layout.columns;
  int actual_index = ((int) button_index - column) + ((int) self->model_info->button_layout.columns - 1 - column);
  return (uint8_t) actual_index;
}


/*
 * Callbacks
 */

static gboolean
save_after_timeout_cb (gpointer data)
{
  BsStreamDeck *self = BS_STREAM_DECK (data);

  BS_ENTRY;

  save_profiles (self);

  self->save_timeout_id = 0;
  BS_RETURN (G_SOURCE_REMOVE);
}


/*
 * Device-specific implementations
 */

/* Mini & Original (gen 1) */

static gboolean
set_button_texture_mini (BsStreamDeck  *self,
                         uint8_t        button,
                         GdkTexture    *texture,
                         GError       **error)
{
  g_autofree unsigned char *payload = NULL;
  g_autofree unsigned char *buffer = NULL;
  const size_t package_size = 1024;
  const size_t header_size = 16;
  size_t bytes_remaining;
  size_t buffer_size;
  size_t page;

  BS_ENTRY;

  if (!bs_icon_renderer_convert_texture (self->icon_renderer, texture, (char **) &buffer, &buffer_size, error))
    BS_RETURN (FALSE);

  payload = g_malloc (sizeof (unsigned char) * package_size);
  payload[0] = 0x02;
  payload[1] = 0x01;
  /* payload[2] set in loop */
  payload[3] = 0;
  /* payload[4] set in loop */
  payload[5] = button + 1;
  payload[6] = 0;
  payload[7] = 0;
  payload[8] = 0;
  payload[9] = 0;
  payload[10] = 0;
  payload[11] = 0;
  payload[12] = 0;
  payload[13] = 0;
  payload[14] = 0;
  payload[15] = 0;

  page = 0;
  bytes_remaining = buffer_size;
  while (bytes_remaining > 0)
    {
      size_t padding_size;
      size_t chunk_size;
      size_t bytes_sent;

      chunk_size = MIN (bytes_remaining, package_size - header_size);

      payload[2] = page;
      payload[4] = chunk_size == bytes_remaining ? 1 : 0;

      bytes_sent = page * (package_size - header_size);
      memcpy (payload + header_size, buffer + bytes_sent, chunk_size);

      padding_size = package_size - header_size - chunk_size;
      if (padding_size > 0)
        memset (payload + header_size + chunk_size, 0, padding_size);

      hid_write (self->handle, payload, package_size);

      bytes_remaining -= chunk_size;
      page++;
    }

  BS_RETURN (TRUE);
}

static gboolean
read_button_states_mini (BsStreamDeck *self)
{
  g_autofree unsigned char *states = NULL;
  const BsStreamDeckButtonLayout *layout;
  size_t states_length;
  uint8_t i;
  int result;

  layout = &self->model_info->button_layout;
  states_length = layout->n_buttons + 1;
  states = g_malloc0 (sizeof (unsigned char) * states_length);

  result = hid_read (self->handle, states, states_length);

  if (result == 0)
    return TRUE;

  for (i = 0; i < layout->n_buttons; i++)
    {
      BsStreamDeckButton *button = g_ptr_array_index (self->buttons, i);
      gboolean pressed = (gboolean) states[i + 1];

      if (bs_stream_deck_button_get_pressed (button) == pressed)
        continue;

      bs_stream_deck_button_set_pressed (button, pressed);

      if (pressed)
        g_signal_emit (self, signals[BUTTON_PRESSED], 0, button);
      else
        g_signal_emit (self, signals[BUTTON_RELEASED], 0, button);
    }

  return TRUE;
}

static void
reset_mini_original (BsStreamDeck *self)
{
  unsigned char reset_command[] = {
    0x0b,
    0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  BS_ENTRY;

  hid_send_feature_report (self->handle, reset_command, sizeof (reset_command));

  BS_EXIT;
}

static char *
get_serial_number_mini_original (BsStreamDeck *self)
{
  unsigned char data[17];
  char *serial;

  BS_ENTRY;

  data[0] = 0x03;

  hid_get_feature_report (self->handle, data, sizeof (data));

  serial = g_malloc0 (sizeof (char) * 13);
  memcpy (serial, &data[5], 12);

  BS_RETURN (serial);
}

static char *
get_firmware_version_mini_original (BsStreamDeck *self)
{
  unsigned char data[17];
  char *serial;

  BS_ENTRY;

  data[0] = 0x04;

  hid_get_feature_report (self->handle, data, sizeof (data));

  serial = g_malloc0 (sizeof (char) * 13);
  memcpy (serial, &data[5], 12);

  BS_RETURN (serial);
}

static void
set_brightness_mini_original (BsStreamDeck *self,
                              gdouble       brightness)
{
  unsigned char b = CLAMP (brightness * 100, 0, 100);
  unsigned char data[] = {
    0x05,
    0x55, 0xaa, 0xd1, 0x01, b   , 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  BS_ENTRY;

  hid_send_feature_report (self->handle, data, sizeof (data));

  BS_EXIT;
}

static gboolean
set_button_texture_original (BsStreamDeck  *self,
                             uint8_t        button,
                             GdkTexture    *texture,
                             GError       **error)
{
  g_autofree unsigned char *payload = NULL;
  g_autofree unsigned char *buffer = NULL;
  const size_t package_size = 8191;
  const size_t header_size = 16;
  size_t bytes_remaining;
  size_t report_size;
  size_t buffer_size;
  size_t page;

  BS_ENTRY;

  if (!bs_icon_renderer_convert_texture (self->icon_renderer, texture, (char **) &buffer, &buffer_size, error))
    BS_RETURN (FALSE);

  report_size = buffer_size / 2;

  /*
   * BMP images have fixed byte sizes for a given width and height, and
   * in this case, a 72x72 BMP image should have exactly 15606 bytes.
   */
  g_assert (buffer_size == 15606);
  g_assert (package_size - header_size >= report_size);

  payload = g_malloc (sizeof (unsigned char) * package_size);
  payload[0] = 0x02;
  payload[1] = 0x01;
  /* payload[2] set in loop */
  payload[3] = 0;
  /* payload[4] set in loop */
  payload[5] = swap_button_index_original (self, button) + 1;
  payload[6] = 0;
  payload[7] = 0;
  payload[8] = 0;
  payload[9] = 0;
  payload[10] = 0;
  payload[11] = 0;
  payload[12] = 0;
  payload[13] = 0;
  payload[14] = 0;
  payload[15] = 0;

  page = 0;
  bytes_remaining = buffer_size;
  while (bytes_remaining > 0)
    {
      size_t padding_size;
      size_t chunk_size;
      size_t bytes_sent;

      chunk_size = MIN (bytes_remaining, report_size);

      payload[2] = page + 1;
      payload[4] = chunk_size == bytes_remaining ? 1 : 0;

      bytes_sent = page * report_size;
      memcpy (payload + header_size, buffer + bytes_sent, chunk_size);

      padding_size = package_size - header_size - chunk_size;
      if (padding_size > 0)
        memset (payload + header_size + chunk_size, 0, padding_size);

      hid_write (self->handle, payload, package_size);

      bytes_remaining -= chunk_size;
      page++;
    }

  BS_RETURN (TRUE);
}

static gboolean
read_button_states_original (BsStreamDeck *self)
{
  g_autofree unsigned char *states = NULL;
  const BsStreamDeckButtonLayout *layout;
  size_t states_length;
  uint8_t i;
  int result;

  layout = &self->model_info->button_layout;
  states_length = layout->n_buttons + 1;
  states = g_malloc0 (sizeof (unsigned char) * states_length);

  result = hid_read (self->handle, states, states_length);

  if (result == 0)
    return TRUE;

  for (i = 0; i < layout->n_buttons; i++)
    {
      uint8_t position = swap_button_index_original (self, i);
      BsStreamDeckButton *button = g_ptr_array_index (self->buttons, position);
      gboolean pressed = (gboolean) states[i + 1];

      if (bs_stream_deck_button_get_pressed (button) == pressed)
        continue;

      bs_stream_deck_button_set_pressed (button, pressed);

      if (pressed)
        g_signal_emit (self, signals[BUTTON_PRESSED], 0, button);
      else
        g_signal_emit (self, signals[BUTTON_RELEASED], 0, button);
    }

  return TRUE;
}

/* 2nd generation */

static void
reset_gen2 (BsStreamDeck *self)
{
  const unsigned char reset_command[] = {
      0x03,
      0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  BS_ENTRY;

  hid_send_feature_report (self->handle, reset_command, sizeof (reset_command));

  BS_EXIT;
}

static char *
get_serial_number_gen2 (BsStreamDeck *self)
{
  unsigned char data[32];
  char *serial;

  BS_ENTRY;

  data[0] = 0x06;

  hid_get_feature_report (self->handle, data, sizeof (data));

  serial = g_malloc0 (sizeof (char) * 31);
  memcpy (serial, &data[2], 30);

  BS_RETURN (serial);
}

static char *
get_firmware_version_gen2 (BsStreamDeck *self)
{
  unsigned char data[32];
  char *serial;

  BS_ENTRY;

  data[0] = 0x05;

  hid_get_feature_report (self->handle, data, sizeof (data));

  serial = g_malloc0 (sizeof (char) * 27);
  memcpy (serial, &data[6], 26);

  BS_RETURN (serial);
}

static void
set_brightness_gen2 (BsStreamDeck *self,
                     gdouble       brightness)
{
  unsigned char b = CLAMP (brightness * 100, 0, 100);
  unsigned char data[] = {
    0x03,
    0x08, b   , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  BS_ENTRY;

  hid_send_feature_report (self->handle, data, sizeof (data));

  BS_EXIT;
}

static gboolean
set_button_texture_gen2 (BsStreamDeck  *self,
                         uint8_t        button,
                         GdkTexture    *texture,
                         GError       **error)
{
  g_autofree unsigned char *payload = NULL;
  g_autofree unsigned char *buffer = NULL;
  const size_t package_size = 1024;
  const size_t header_size = 8;
  size_t bytes_remaining;
  size_t buffer_size;
  size_t page;

  BS_ENTRY;

  if (!bs_icon_renderer_convert_texture (self->icon_renderer, texture, (char **) &buffer, &buffer_size, error))
    BS_RETURN (FALSE);

  payload = g_malloc (package_size * sizeof (unsigned char));
  payload[0] = 0x02;
  payload[1] = 0x07;
  payload[2] = button;

  page = 0;
  bytes_remaining = buffer_size;
  while (bytes_remaining > 0)
    {
      size_t padding_size;
      size_t chunk_size;
      size_t bytes_sent;

      chunk_size = MIN (bytes_remaining, package_size - header_size);

      payload[3] = chunk_size == bytes_remaining ? 1 : 0;
      payload[4] = chunk_size & 0xff;
      payload[5] = chunk_size >> 8;
      payload[6] = page & 0xff;
      payload[7] = page >> 8;

      bytes_sent = page * (package_size - header_size);
      memcpy (payload + header_size, buffer + bytes_sent, chunk_size);

      padding_size = package_size - header_size - chunk_size;
      if (padding_size > 0)
        memset (payload + header_size + chunk_size, 0, padding_size);

      hid_write (self->handle, payload, package_size * sizeof (unsigned char));

      bytes_remaining -= chunk_size;
      page++;
    }

  BS_RETURN (TRUE);
}

static gboolean
read_button_states_gen2 (BsStreamDeck *self)
{
  g_autofree unsigned char *states = NULL;
  const BsStreamDeckButtonLayout *layout;
  size_t states_length;
  uint8_t i;
  int result;

  layout = &self->model_info->button_layout;
  states_length = layout->n_buttons + 4;
  states = g_malloc0 (sizeof (unsigned char) * states_length);

  result = hid_read (self->handle, states, states_length);

  if (result == 0)
    return TRUE;

  for (i = 0; i < layout->n_buttons; i++)
    {
      BsStreamDeckButton *button = g_ptr_array_index (self->buttons, i);
      gboolean pressed = (gboolean) states[i + 4];

      if (bs_stream_deck_button_get_pressed (button) == pressed)
        continue;

      bs_stream_deck_button_set_pressed (button, pressed);

      if (pressed)
        g_signal_emit (self, signals[BUTTON_PRESSED], 0, button);
      else
        g_signal_emit (self, signals[BUTTON_RELEASED], 0, button);
    }

  return TRUE;
}

/* noops for devices without visual feedback */

static void
set_brightness_pedal (BsStreamDeck *self,
                     gdouble       brightness)
{
  BS_ENTRY;
  BS_EXIT;
}

static gboolean
set_button_texture_pedal (BsStreamDeck  *self,
                         uint8_t        button,
                         GdkTexture    *texture,
                         GError       **error)
{
  BS_ENTRY;
  BS_RETURN (TRUE);
}

static void
reset_pedal (BsStreamDeck *self)
{
  BS_ENTRY;
  BS_EXIT;
}

static const StreamDeckModelInfo models_vtable[] = {
  {
    .product_id = STREAMDECK_MINI_PRODUCT_ID,
    /* Translators: this is a product name. In most cases, it is not translated.
     * Please verify if Elgato translates their product names on your locale.
     */
    .name = N_("Stream Deck Mini"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 6,
      .rows = 2,
      .columns = 3,
      .icon_size = 80,
    },
    .icon_layout = {
      .width = 80,
      .height = 80,
      .format = BS_ICON_FORMAT_BMP,
      .flags = BS_ICON_RENDERER_FLAG_FLIP_Y | BS_ICON_RENDERER_FLAG_ROTATE_90,
    },
    .reset = reset_mini_original,
    .get_serial_number = get_serial_number_mini_original,
    .get_firmware_version = get_firmware_version_mini_original,
    .set_brightness = set_brightness_mini_original,
    .set_button_texture = set_button_texture_mini,
    .read_button_states = read_button_states_mini,
  },
  {
    .product_id = STREAMDECK_ORIGINAL_PRODUCT_ID,
    /* Translators: this is a product name. In most cases, it is not translated.
     * Please verify if Elgato translates their product names on your locale.
     */
    .name = N_("Stream Deck"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 15,
      .rows = 3,
      .columns = 5,
      .icon_size = 72,
    },
    .icon_layout = {
      .width = 72,
      .height = 72,
      .format = BS_ICON_FORMAT_BMP,
      .flags = BS_ICON_RENDERER_FLAG_FLIP_X | BS_ICON_RENDERER_FLAG_FLIP_Y,
    },
    .reset = reset_mini_original,
    .get_serial_number = get_serial_number_mini_original,
    .get_firmware_version = get_firmware_version_mini_original,
    .set_brightness = set_brightness_mini_original,
    .set_button_texture = set_button_texture_original,
    .read_button_states = read_button_states_original,
  },
  {
    .product_id = STREAMDECK_ORIGINAL_V2_PRODUCT_ID,
    /* Translators: this is a product name. In most cases, it is not translated.
     * Please verify if Elgato translates their product names on your locale.
     */
    .name = N_("Stream Deck"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 15,
      .rows = 3,
      .columns = 5,
      .icon_size = 72,
    },
    .icon_layout = {
      .width = 72,
      .height = 72,
      .format = BS_ICON_FORMAT_JPEG,
      .flags = BS_ICON_RENDERER_FLAG_FLIP_X | BS_ICON_RENDERER_FLAG_FLIP_Y,
    },
    .reset = reset_gen2,
    .get_serial_number = get_serial_number_gen2,
    .get_firmware_version = get_firmware_version_gen2,
    .set_brightness = set_brightness_gen2,
    .set_button_texture = set_button_texture_gen2,
    .read_button_states = read_button_states_gen2,
  },
  {
    .product_id = STREAMDECK_XL_PRODUCT_ID,
    /* Translators: this is a product name. In most cases, it is not translated.
     * Please verify if Elgato translates their product names on your locale.
     */
    .name = N_("Stream Deck XL"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 32,
      .rows = 4,
      .columns = 8,
      .icon_size = 96,
    },
    .icon_layout = {
      .width = 96,
      .height = 96,
      .format = BS_ICON_FORMAT_JPEG,
      .flags = BS_ICON_RENDERER_FLAG_FLIP_X | BS_ICON_RENDERER_FLAG_FLIP_Y,
    },
    .reset = reset_gen2,
    .get_serial_number = get_serial_number_gen2,
    .get_firmware_version = get_firmware_version_gen2,
    .set_brightness = set_brightness_gen2,
    .set_button_texture = set_button_texture_gen2,
    .read_button_states = read_button_states_gen2,
  },
  {
    .product_id = STREAMDECK_XL_V2_PRODUCT_ID,
    /* Translators: this is a product name. In most cases, it is not translated.
     * Please verify if Elgato translates their product names on your locale.
     */
    .name = N_("Stream Deck XL"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 32,
      .rows = 4,
      .columns = 8,
      .icon_size = 96,
    },
    .icon_layout = {
      .width = 96,
      .height = 96,
      .format = BS_ICON_FORMAT_JPEG,
      .flags = BS_ICON_RENDERER_FLAG_FLIP_X | BS_ICON_RENDERER_FLAG_FLIP_Y,
    },
    .reset = reset_gen2,
    .get_serial_number = get_serial_number_gen2,
    .get_firmware_version = get_firmware_version_gen2,
    .set_brightness = set_brightness_gen2,
    .set_button_texture = set_button_texture_gen2,
    .read_button_states = read_button_states_gen2,
  },
  {
    .product_id = STREAMDECK_MK2_PRODUCT_ID,
    /* Translators: this is a product name. In most cases, it is not translated.
     * Please verify if Elgato translates their product names on your locale.
     */
    .name = N_("Stream Deck MK.2"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 15,
      .rows = 3,
      .columns = 5,
      .icon_size = 72,
    },
    .icon_layout = {
      .width = 72,
      .height = 72,
      .format = BS_ICON_FORMAT_JPEG,
      .flags = BS_ICON_RENDERER_FLAG_FLIP_X | BS_ICON_RENDERER_FLAG_FLIP_Y,
    },
    .reset = reset_gen2,
    .get_serial_number = get_serial_number_gen2,
    .get_firmware_version = get_firmware_version_gen2,
    .set_brightness = set_brightness_gen2,
    .set_button_texture = set_button_texture_gen2,
    .read_button_states = read_button_states_gen2,
  },
  {
    .product_id = STREAMDECK_PEDAL_PRODUCT_ID,
    /* Translators: this is a product name. In most cases, it is not translated.
     * Please verify if Elgato translates their product names on your locale.
     */
    .name = N_("Stream Deck Pedal"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 3,
      .rows = 1,
      .columns = 3,
      .icon_size = 96,
    },
    .icon_layout = {
      .width = 96,
      .height = 96,
      .format = BS_ICON_FORMAT_JPEG,
      .flags = BS_ICON_RENDERER_FLAG_NONE,
    },
    .reset = reset_pedal,
    .get_serial_number = get_serial_number_gen2,
    .get_firmware_version = get_firmware_version_gen2,
    .set_brightness = set_brightness_pedal,
    .set_button_texture = set_button_texture_pedal,
    .read_button_states = read_button_states_gen2,
  },
};


/*
 * Fake device for testing
 */

static void
reset_fake (BsStreamDeck *self)
{
}

static char *
get_serial_number_fake (BsStreamDeck *self)
{
  return g_strdup ("feaneron-hangar-xl-serial");
}

static char *
get_firmware_version_fake (BsStreamDeck *self)
{
  return g_strdup ("feaneron-hangar-xl-firmware-version");
}

static void
set_brightness_fake (BsStreamDeck *self,
                     double        brightness)
{
}

static gboolean
set_button_texture_fake (BsStreamDeck  *self,
                         uint8_t        button,
                         GdkTexture    *texture,
                         GError       **error)
{
  return TRUE;
}

static gboolean
read_button_states_fake (BsStreamDeck *self)
{
  return TRUE;
}

static const StreamDeckModelInfo fake_models_vtable[] = {
  {
    .product_id = 0x0001,
    .name = N_("Feaneron Hangar Original"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 15,
      .rows = 3,
      .columns = 5,
      .icon_size = 72,
    },
    .icon_layout = {
      .width = 72,
      .height = 72,
      .format = BS_ICON_FORMAT_JPEG,
      .flags = BS_ICON_RENDERER_FLAG_FLIP_X | BS_ICON_RENDERER_FLAG_FLIP_Y,
    },
    .reset = reset_fake,
    .get_serial_number = get_serial_number_fake,
    .get_firmware_version = get_firmware_version_fake,
    .set_brightness = set_brightness_fake,
    .set_button_texture = set_button_texture_fake,
    .read_button_states = read_button_states_fake,
  },
  {
    .product_id = 0x0001,
    .name = N_("Feaneron Hangar XL"),
    .icon_name = "input-dialpad-symbolic",
    .button_layout = {
      .n_buttons = 32,
      .rows = 4,
      .columns = 8,
      .icon_size = 96,
    },
    .icon_layout = {
      .width = 96,
      .height = 96,
      .format = BS_ICON_FORMAT_JPEG,
      .flags = BS_ICON_RENDERER_FLAG_FLIP_X | BS_ICON_RENDERER_FLAG_FLIP_Y,
    },
    .reset = reset_fake,
    .get_serial_number = get_serial_number_fake,
    .get_firmware_version = get_firmware_version_fake,
    .set_brightness = set_brightness_fake,
    .set_button_texture = set_button_texture_fake,
    .read_button_states = read_button_states_fake,
  },
};

/*
 * GSource
 */

static gboolean
stream_deck_source_dispatch (GSource     *source,
                             GSourceFunc  callback,
                             gpointer     user_data)
{
  StreamDeckSource *stream_deck_source = (StreamDeckSource *)source;
  BsStreamDeck *self = stream_deck_source->stream_deck;
  gint64 current_time;
  gint64 expiration;

  self->model_info->read_button_states (self);

  current_time = g_source_get_time (source);
  expiration = current_time + (guint64) POLL_RATE_MS * 1000;
  g_source_set_ready_time (source, expiration);

  return TRUE;
}

GSourceFuncs stream_deck_source_funcs =
{
  NULL, /* prepare */
  NULL, /* check */
  stream_deck_source_dispatch,
  NULL, NULL, NULL,
};

static GSource *
stream_deck_source_new (BsStreamDeck *self)
{
  StreamDeckSource *stream_deck_source;
  GSource *source;

  source = g_source_new (&stream_deck_source_funcs, sizeof (StreamDeckSource));
  stream_deck_source = (StreamDeckSource *)source;
  stream_deck_source->stream_deck = self;

  g_source_set_ready_time (source, g_get_monotonic_time ());

  return source;
}


/*
 * GInitable interface
 */

static gboolean
bs_stream_deck_initable_init (GInitable     *initable,
                              GCancellable  *cancellable,
                              GError       **error)
{
  BsStreamDeck *self = BS_STREAM_DECK (initable);
  size_t i;

  BS_ENTRY;

  /* Short-circuit fake devices here */
  if (self->fake)
    {
      static int fake_index = 0;
      self->model_info = &fake_models_vtable[fake_index++ % G_N_ELEMENTS (fake_models_vtable)];
      BS_GOTO (out);
    }

  if (!self->device)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "No device");
      BS_RETURN (FALSE);
    }

  if (g_usb_device_get_vid (self->device) != ELGATO_SYSTEMS_VENDOR_ID)
    {
      g_set_error (error,
                   BS_STREAM_DECK_ERROR,
                   BS_STREAM_DECK_ERROR_UNRECOGNIZED,
                   "Not an Elgato device");
      BS_RETURN (FALSE);
    }

  for (i = 0; i < G_N_ELEMENTS (models_vtable); i++)
    {
      if (g_usb_device_get_pid (self->device) == models_vtable[i].product_id)
        {
          self->model_info = &models_vtable[i];
          break;
        }
    }

  if (!self->model_info)
    {
      g_set_error (error,
                   BS_STREAM_DECK_ERROR,
                   BS_STREAM_DECK_ERROR_UNRECOGNIZED,
                   "Not a recognized Stream Deck device");
      BS_RETURN (FALSE);
    }

  self->handle = hid_open (g_usb_device_get_vid (self->device),
                           g_usb_device_get_pid (self->device),
                           NULL);

  if (!self->handle)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Failed to open Stream Deck device");
      BS_RETURN (FALSE);
    }

  hid_set_nonblocking (self->handle, TRUE);

  self->poll_source = stream_deck_source_new (self);

out:
  self->serial_number = self->model_info->get_serial_number (self);
  self->firmware_version = self->model_info->get_firmware_version (self);
  self->icon_renderer = bs_icon_renderer_new (&self->model_info->icon_layout);
  self->icon = g_themed_icon_new (self->model_info->icon_name);

  for (i = 0; i < self->model_info->button_layout.n_buttons; i++)
    g_ptr_array_add (self->buttons, bs_stream_deck_button_new (self, i));

  self->initialized = TRUE;

  BS_RETURN (TRUE);
}

static void
g_initable_iface_init (GInitableIface *iface)
{
  iface->init = bs_stream_deck_initable_init;
}

/*
 * GObject overrides
 */

static void
bs_stream_deck_finalize (GObject *object)
{
  BsStreamDeck *self = (BsStreamDeck *)object;

  BS_ENTRY;

  if (self->initialized)
    {
      save_profiles (self);
      bs_stream_deck_reset (self);
    }

  if (self->device)
    g_usb_device_close (self->device, NULL);

  if (self->poll_source)
    g_source_destroy (self->poll_source);
  g_clear_pointer (&self->poll_source, g_source_unref);

  g_clear_handle_id (&self->save_timeout_id, g_source_remove);
  g_clear_pointer (&self->serial_number, g_free);
  g_clear_pointer (&self->buttons, g_ptr_array_unref);
  g_clear_pointer (&self->handle, hid_close);
  g_queue_free_full (self->active_pages, g_object_unref);
  g_clear_object (&self->device);
  g_clear_object (&self->profiles);

  G_OBJECT_CLASS (bs_stream_deck_parent_class)->finalize (object);

  BS_EXIT;
}

static void
bs_stream_deck_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BsStreamDeck *self = BS_STREAM_DECK (object);

  switch (prop_id)
    {
    case PROP_ACTIVE_PAGE:
      g_value_set_object (value, bs_stream_deck_get_active_page (self));
      break;

    case PROP_ACTIVE_PROFILE:
      g_value_set_object (value, self->active_profile);
      break;

    case PROP_BRIGHTNESS:
      g_value_set_double (value, self->brightness);
      break;

    case PROP_DEVICE:
      g_value_set_object (value, self->device);
      break;

    case PROP_ICON:
      g_value_set_object (value, self->icon);
      break;

    case PROP_NAME:
      g_value_set_string (value, bs_stream_deck_get_name (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BsStreamDeck *self = BS_STREAM_DECK (object);

  switch (prop_id)
    {
    case PROP_BRIGHTNESS:
      bs_stream_deck_set_brightness (self, g_value_get_double (value));
      break;

    case PROP_DEVICE:
      g_assert (self->device == NULL);
      self->device = g_value_dup_object (value);
      break;

    case PROP_FAKE:
      self->fake = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_class_init (BsStreamDeckClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_stream_deck_finalize;
  object_class->get_property = bs_stream_deck_get_property;
  object_class->set_property = bs_stream_deck_set_property;

  properties[PROP_ACTIVE_PAGE] = g_param_spec_object ("active-page", NULL, NULL,
                                                      BS_TYPE_PAGE,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_ACTIVE_PROFILE] = g_param_spec_object ("active-profile", NULL, NULL,
                                                         BS_TYPE_PROFILE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_BRIGHTNESS] = g_param_spec_double ("brightness", NULL, NULL,
                                                     0.0, 1.0, 0.5,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_DEVICE] = g_param_spec_object ("device", NULL, NULL,
                                                 G_USB_TYPE_DEVICE,
                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_FAKE] = g_param_spec_boolean ("fake", NULL, NULL,
                                                FALSE,
                                                G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_ICON] = g_param_spec_object ("icon", NULL, NULL,
                                               G_TYPE_ICON,
                                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_NAME] = g_param_spec_string ("name", NULL, NULL, NULL,
                                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals[BUTTON_PRESSED] = g_signal_new ("button-pressed",
                                          BS_TYPE_STREAM_DECK,
                                          G_SIGNAL_RUN_LAST,
                                          0, NULL, NULL, NULL,
                                          G_TYPE_NONE,
                                          1,
                                          BS_TYPE_STREAM_DECK_BUTTON);

  signals[BUTTON_RELEASED] = g_signal_new ("button-released",
                                           BS_TYPE_STREAM_DECK,
                                           G_SIGNAL_RUN_LAST,
                                           0, NULL, NULL, NULL,
                                           G_TYPE_NONE,
                                           1,
                                           BS_TYPE_STREAM_DECK_BUTTON);
}

static void
bs_stream_deck_init (BsStreamDeck *self)
{
  self->buttons = g_ptr_array_new_with_free_func (g_object_unref);
  self->profiles = g_list_store_new (BS_TYPE_PROFILE);
  self->active_pages = g_queue_new ();
}

BsStreamDeck *
bs_stream_deck_new (GUsbDevice  *gusb_device,
                    GError     **error)
{
  return g_initable_new (BS_TYPE_STREAM_DECK,
                         NULL,
                         error,
                         "device", gusb_device,
                         NULL);
}

BsStreamDeck *
bs_stream_deck_new_fake (GError **error)
{
  return g_initable_new (BS_TYPE_STREAM_DECK,
                         NULL,
                         error,
                         "fake", TRUE,
                         NULL);
}

void
bs_stream_deck_reset (BsStreamDeck *self)
{
  g_return_if_fail (BS_IS_STREAM_DECK (self));
  g_return_if_fail (self->model_info->reset != NULL);

  self->model_info->reset (self);
}

GUsbDevice *
bs_stream_deck_get_device (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return self->device;
}

const char *
bs_stream_deck_get_name (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return _(self->model_info->name);
}

const char *
bs_stream_deck_get_serial_number (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return self->serial_number;
}

const char *
bs_stream_deck_get_firmware_version (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return self->firmware_version;
}

GIcon *
bs_stream_deck_get_icon (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return self->icon;
}

/**
 * bs_stream_deck_get_button_layout:
 * @self: a #BsStreamDeck
 *
 * Retrieves the #BS of @self.
 *
 * Returns: (transfer none): a #BsIconRenderer
 */
const BsStreamDeckButtonLayout *
bs_stream_deck_get_button_layout (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return &self->model_info->button_layout;
}

double
bs_stream_deck_get_brightness (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), 0.0);

  return self->brightness;
}

/**
 * bs_stream_deck_set_brightness:
 * @self: a #BsStreamDeck
 * @brightness: a double between and including 0.0 and 1.0
 *
 * Sets the brightness of @self to @brightness.
 */
void
bs_stream_deck_set_brightness (BsStreamDeck *self,
                               double        brightness)
{
  g_return_if_fail (BS_IS_STREAM_DECK (self));
  g_return_if_fail (brightness >= 0.0 && brightness <= 1.0);
  g_return_if_fail (self->model_info->set_brightness != NULL);

  if (G_APPROX_VALUE (self->brightness, brightness, FLT_EPSILON))
    return;

  self->brightness = brightness;
  self->model_info->set_brightness (self, brightness);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BRIGHTNESS]);
}

/**
 * bs_stream_deck_set_button_texture:
 * @self: a #BsStreamDeck
 *
 * Retrieves the #BsIconRenderer of @self.
 *
 * Returns: (transfer none): a #BsIconRenderer
 */
gboolean
bs_stream_deck_set_button_icon (BsStreamDeck  *self,
                                uint8_t        button,
                                BsIcon        *icon,
                                GError       **error)
{
  g_autoptr (GdkTexture) texture = NULL;

  g_return_val_if_fail (BS_IS_STREAM_DECK (self), FALSE);
  g_return_val_if_fail (button < self->model_info->button_layout.n_buttons, FALSE);
  g_return_val_if_fail (self->model_info->set_button_texture != NULL, FALSE);

  texture = bs_icon_renderer_compose_icon (self->icon_renderer,
                                           BS_ICON_COMPOSE_FLAG_NONE,
                                           icon,
                                           error);

  if (!texture)
    return FALSE;

  return self->model_info->set_button_texture (self, button, texture, error);
}

/**
 * bs_stream_deck_get_icon_renderer:
 * @self: a #BsStreamDeck
 *
 * Retrieves the #BsIconRenderer of @self.
 *
 * Returns: (transfer none): a #BsIconRenderer
 */
BsIconRenderer *
bs_stream_deck_get_icon_renderer (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return self->icon_renderer;
}

/**
 * bs_stream_deck_get_button:
 * @self: a #BsStreamDeck
 * @position: position of the button
 *
 * Retrieves the #BsStreamDeckButton at @position.
 *
 * Returns: (transfer none): a #BsStreamDeckButton
 */
BsStreamDeckButton *
bs_stream_deck_get_button (BsStreamDeck *self,
                           uint8_t       position)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);
  g_return_val_if_fail (position < self->model_info->button_layout.n_buttons, NULL);

  return g_ptr_array_index (self->buttons, position);
}

GListModel *
bs_stream_deck_get_profiles (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return G_LIST_MODEL (self->profiles);
}

BsProfile *
bs_stream_deck_get_active_profile (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return self->active_profile;
}

void
bs_stream_deck_load_profile (BsStreamDeck *self,
                             BsProfile    *profile)
{
  g_return_if_fail (BS_IS_STREAM_DECK (self));
  g_return_if_fail (g_list_store_find (self->profiles, profile, NULL));

  BS_ENTRY;

  if (self->active_profile == profile)
    BS_RETURN ();

  g_queue_clear_full (self->active_pages, g_object_unref);

  self->active_profile = profile;

  bs_stream_deck_set_brightness (self, bs_profile_get_brightness (profile));
  bs_stream_deck_push_page (self, bs_profile_get_root_page (profile));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIVE_PROFILE]);

  BS_EXIT;
}

BsPage *
bs_stream_deck_get_active_page (BsStreamDeck *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK (self), NULL);

  return g_queue_peek_head (self->active_pages);
}

void
bs_stream_deck_push_page (BsStreamDeck  *self,
                          BsPage        *page)
{
  g_return_if_fail (BS_IS_STREAM_DECK (self));
  g_return_if_fail (BS_IS_PAGE (page));
  g_return_if_fail (bs_page_get_profile (page) == self->active_profile);
  g_return_if_fail (g_queue_find (self->active_pages, page) == NULL);
  g_return_if_fail (bs_page_get_parent (page) == bs_stream_deck_get_active_page (self));

  BS_ENTRY;

  if (g_queue_get_length (self->active_pages) > 0)
    bs_page_update_all_items (g_queue_peek_head (self->active_pages));

  g_queue_push_head (self->active_pages, g_object_ref (page));

  load_active_page (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIVE_PAGE]);

  BS_EXIT;
}

void
bs_stream_deck_pop_page (BsStreamDeck *self)
{
  g_autoptr (BsPage) page = NULL;
  unsigned int i;

  g_return_if_fail (BS_IS_STREAM_DECK (self));
  g_return_if_fail (g_queue_get_length (self->active_pages) > 1);

  BS_ENTRY;

  page = g_queue_pop_head (self->active_pages);

  for (i = 0; i < self->model_info->button_layout.n_buttons; i++)
    bs_page_update_item_from_button (page, g_ptr_array_index (self->buttons, i));

  bs_page_update_all_items (g_queue_peek_head (self->active_pages));

  load_active_page (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIVE_PAGE]);

  BS_EXIT;
}

void
bs_stream_deck_load (BsStreamDeck *self)
{
  g_return_if_fail (BS_IS_STREAM_DECK (self));
  g_return_if_fail (!self->loaded);

  if (!self->fake)
    g_source_attach (self->poll_source, NULL);

  load_profiles (self);

  self->loaded = TRUE;
}

void
bs_stream_deck_save (BsStreamDeck *self)
{
  g_return_if_fail (BS_IS_STREAM_DECK (self));

  BS_ENTRY;

  if (self->save_timeout_id == 0)
    self->save_timeout_id = g_timeout_add_seconds (5, save_after_timeout_cb, self);

  BS_EXIT;
}
