/* bs-page.c
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

#include "bs-action-private.h"
#include "bs-empty-action.h"
#include "bs-page.h"
#include "bs-page-item.h"
#include "bs-profile.h"
#include "bs-stream-deck-button.h"

#include <libpeas/peas.h>

struct _BsPage
{
  GObject parent_instance;

  GPtrArray *items;
  BsProfile *profile;
  BsPage *parent;
};

G_DEFINE_FINAL_TYPE (BsPage, bs_page, G_TYPE_OBJECT)

enum
{
  PROP_0,
  PROP_PARENT,
  PROP_PROFILE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/*
 * Auxiliary methods
 */

static inline BsPageItem *
get_item (BsPage       *self,
          unsigned int  position)
{
  if (!self->items || position >= self->items->len)
    return NULL;

  return g_ptr_array_index (self->items, position);
}


/*
 * GObject overrides
 */

static void
bs_page_finalize (GObject *object)
{
  BsPage *self = (BsPage *)object;

  g_clear_pointer (&self->items, g_ptr_array_unref);

  G_OBJECT_CLASS (bs_page_parent_class)->finalize (object);
}

static void
bs_page_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  BsPage *self = BS_PAGE (object);

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, self->parent);
      break;

    case PROP_PROFILE:
      g_value_set_object (value, self->profile);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_page_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  BsPage *self = BS_PAGE (object);

  switch (prop_id)
    {
    case PROP_PARENT:
      g_assert (self->parent == NULL);
      self->parent = g_value_get_object (value);
      break;

    case PROP_PROFILE:
      g_assert (self->profile == NULL);
      self->profile = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_page_class_init (BsPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_page_finalize;
  object_class->get_property = bs_page_get_property;
  object_class->set_property = bs_page_set_property;

  properties[PROP_PARENT] = g_param_spec_object ("parent", NULL, NULL,
                                                 BS_TYPE_PAGE,
                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_PROFILE] = g_param_spec_object ("profile", NULL, NULL,
                                                  BS_TYPE_PROFILE,
                                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_page_init (BsPage *self)
{
  self->items = g_ptr_array_new_with_free_func (g_object_unref);
}

BsPage *
bs_page_new (void)
{
  return g_object_new (BS_TYPE_PAGE, NULL);
}

BsPage *
bs_page_new_empty (BsProfile *profile,
                   BsPage    *parent)
{
  return g_object_new (BS_TYPE_PAGE,
                       "profile", profile,
                       "parent", parent,
                       NULL);
}

BsPage *
bs_page_new_from_json (BsProfile *profile,
                       BsPage    *parent,
                       JsonNode  *node)
{
  g_autoptr (BsPage) page = NULL;
  JsonArray *array;
  guint i;

  page = bs_page_new_empty (profile, parent);

  if (!JSON_NODE_HOLDS_ARRAY (node))
    {
      g_warning ("JSON node is not an array");
      goto out;
    }

  array = json_node_get_array (node);
  for (i = 0; i < json_array_get_length (array); i++)
    {
      JsonNode *button_node = json_array_get_element (array, i);

      g_ptr_array_insert (page->items, i, bs_page_item_new_from_json (page, button_node));
    }

out:
  return g_steal_pointer (&page);
}

JsonNode *
bs_page_to_json (BsPage *self)
{
  g_autoptr (JsonBuilder) builder = NULL;
  unsigned int i;

  g_return_val_if_fail (BS_IS_PAGE (self), NULL);

  builder = json_builder_new ();

  json_builder_begin_array (builder);

  for (i = 0; i < self->items->len; i++)
    {
      BsPageItem *item;

      item = g_ptr_array_index (self->items, i);
      json_builder_add_value (builder, bs_page_item_to_json (item));
    }

  json_builder_end_array (builder);

  return json_builder_get_root (builder);
}

BsProfile *
bs_page_get_profile (BsPage *self)
{
  g_return_val_if_fail (BS_IS_PAGE (self), NULL);

  return self->profile;
}

void
bs_page_update_button (BsPage             *self,
                       BsStreamDeckButton *button)
{
  BsPageItem *item;
  BsAction *action;
  GType action_type;
  uint8_t position;

  g_return_if_fail (BS_IS_PAGE (self));
  g_return_if_fail (BS_IS_STREAM_DECK_BUTTON (button));

  position = bs_stream_deck_button_get_position (button);
  item = get_item (self, position);

  if (!item)
    {
      item = bs_page_item_new (self);
      g_ptr_array_insert (self->items, position, item);
    }

  action = bs_stream_deck_button_get_action (button);
  action_type = G_OBJECT_TYPE (action);

  if (action_type == BS_TYPE_EMPTY_ACTION)
    {
      bs_page_item_set_item_type (item, BS_PAGE_ITEM_EMPTY);
    }
  else
    {
      BsActionFactory *action_factory;
      PeasPluginInfo *plugin_info;

      action_factory = bs_action_get_factory (action);
      plugin_info = peas_extension_base_get_plugin_info (PEAS_EXTENSION_BASE (action_factory));

      bs_page_item_set_item_type (item, BS_PAGE_ITEM_ACTION);
      bs_page_item_set_factory (item, peas_plugin_info_get_module_name (plugin_info));
      bs_page_item_set_action (item, bs_action_get_id (action));
      bs_page_item_set_settings (item, bs_action_serialize_settings (action));
    }
}

gboolean
bs_page_realize (BsPage    *self,
                 uint8_t    position,
                 BsIcon   **out_custom_icon,
                 BsAction **out_action,
                 GError   **error)
{
  BsPageItem *item;

  g_return_val_if_fail (BS_IS_PAGE (self), FALSE);
  g_return_val_if_fail (out_custom_icon != NULL, FALSE);
  g_return_val_if_fail (out_action != NULL, FALSE);

  item = get_item (self, position);

  if (!item)
    {
      *out_custom_icon = NULL;
      *out_action = bs_empty_action_new ();
      return FALSE;
    }

  return bs_page_item_realize (item, out_custom_icon, out_action, error);
}
