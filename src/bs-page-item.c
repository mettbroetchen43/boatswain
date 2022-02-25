/* bs-page-item.c
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
#include "bs-action-factory.h"
#include "bs-application.h"
#include "bs-enum-types.h"
#include "bs-empty-action.h"
#include "bs-page-item.h"

#include <libpeas/peas.h>

struct _BsPageItem
{
  GObject parent_instance;

  char *action;
  BsPageItemType item_type;
  char *factory;
  JsonNode *settings;
};

static void json_serializable_iface_init (JsonSerializableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BsPageItem, bs_page_item, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE, json_serializable_iface_init))

enum
{
  PROP_0,
  PROP_ACTION,
  PROP_FACTORY,
  PROP_SETTINGS,
  PROP_TYPE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

/*
 * Auxiliary methods
 */

typedef struct
{
  const char *factory_id;
  BsActionFactory *factory;
} FindFactoryData;

static void
find_action_factory_cb (PeasExtensionSet *set,
                        PeasPluginInfo   *info,
                        PeasExtension    *extension,
                        gpointer          data)
{
  FindFactoryData *find_data = data;

  if (g_strcmp0 (peas_plugin_info_get_module_name (info), find_data->factory_id) == 0)
    find_data->factory = BS_ACTION_FACTORY (extension);
}

static BsActionFactory *
get_action_factory (const char *factory_id)
{
  FindFactoryData find_data;
  GApplication *application;

  find_data.factory_id = factory_id;
  find_data.factory = NULL;

  application = g_application_get_default ();
  peas_extension_set_foreach (bs_application_get_action_factory_set (BS_APPLICATION (application)),
                              find_action_factory_cb,
                              &find_data);

  return find_data.factory;
}



/*
 * JsonSerializable interface
 */

static gboolean
bs_page_item_deserialize_property (JsonSerializable *serializable,
                                   const char       *property_name,
                                   GValue           *value,
                                   GParamSpec       *pspec,
                                   JsonNode         *property_node)
{
  if (g_strcmp0 (property_name, "settings") == 0)
    {
      g_value_set_boxed (value, property_node);
      return TRUE;
    }

  return json_serializable_default_deserialize_property (serializable,
                                                         property_name,
                                                         value,
                                                         pspec,
                                                         property_node);
}

static JsonNode *
bs_page_item_serialize_property (JsonSerializable *serializable,
                                 const char       *property_name,
                                 const GValue     *value,
                                 GParamSpec       *pspec)
{
  if (g_strcmp0 (property_name, "settings") == 0)
    return g_value_dup_boxed (value);

  return json_serializable_default_serialize_property (serializable,
                                                       property_name,
                                                       value,
                                                       pspec);
}

static GParamSpec **
bs_page_item_list_properties (JsonSerializable *serializable,
                              unsigned int     *n_properties)
{
  BsPageItem *self = BS_PAGE_ITEM (serializable);
  g_autoptr (GPtrArray) props = NULL;

  props = g_ptr_array_new ();

  g_ptr_array_add (props, properties[PROP_TYPE]);

  switch (self->item_type)
    {
    case BS_PAGE_ITEM_EMPTY:
      break;

    case BS_PAGE_ITEM_ACTION:
      g_ptr_array_add (props, properties[PROP_ACTION]);
      g_ptr_array_add (props, properties[PROP_FACTORY]);
      g_ptr_array_add (props, properties[PROP_SETTINGS]);
      break;
    }

  if (n_properties)
    *n_properties = props->len;

  g_ptr_array_add (props, NULL);

  return (GParamSpec **) g_ptr_array_free (g_steal_pointer (&props), FALSE);
}

static void
json_serializable_iface_init (JsonSerializableIface *iface)
{
  iface->list_properties = bs_page_item_list_properties;
  iface->serialize_property = bs_page_item_serialize_property;
  iface->deserialize_property = bs_page_item_deserialize_property;
}


/*
 * GObject overrides
 */

static void
bs_page_item_finalize (GObject *object)
{
  BsPageItem *self = (BsPageItem *)object;

  g_clear_pointer (&self->action, g_free);
  g_clear_pointer (&self->factory, g_free);
  g_clear_pointer (&self->settings, json_node_unref);

  G_OBJECT_CLASS (bs_page_item_parent_class)->finalize (object);
}

static void
bs_page_item_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BsPageItem *self = BS_PAGE_ITEM (object);

  switch (prop_id)
    {
    case PROP_ACTION:
      g_value_set_string (value, self->action);
      break;

    case PROP_FACTORY:
      g_value_set_string (value, self->factory);
      break;

    case PROP_SETTINGS:
      g_value_set_boxed (value, self->settings);
      break;

    case PROP_TYPE:
      g_value_set_enum (value, self->item_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_page_item_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BsPageItem *self = BS_PAGE_ITEM (object);

  switch (prop_id)
    {
    case PROP_ACTION:
      bs_page_item_set_action (self, g_value_get_string (value));
      break;

    case PROP_FACTORY:
      bs_page_item_set_factory (self, g_value_get_string (value));
      break;

    case PROP_SETTINGS:
      bs_page_item_set_settings (self, g_value_get_boxed (value));
      break;

    case PROP_TYPE:
      bs_page_item_set_item_type (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_page_item_class_init (BsPageItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_page_item_finalize;
  object_class->get_property = bs_page_item_get_property;
  object_class->set_property = bs_page_item_set_property;

  properties[PROP_ACTION] = g_param_spec_string ("action", NULL, NULL,
                                                 "",
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_FACTORY] = g_param_spec_string ("factory", NULL, NULL,
                                                  "",
                                                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_SETTINGS] = g_param_spec_boxed ("settings", NULL, NULL,
                                                  JSON_TYPE_NODE,
                                                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_TYPE] = g_param_spec_enum ("type", NULL, NULL,
                                             BS_TYPE_PAGE_ITEM_TYPE,
                                             BS_PAGE_ITEM_EMPTY,
                                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_page_item_init (BsPageItem *self)
{
}

BsPageItem *
bs_page_item_new (void)
{
  return g_object_new (BS_TYPE_PAGE_ITEM, NULL);
}

const char *
bs_page_item_get_action (BsPageItem *self)
{
  g_return_val_if_fail (BS_IS_PAGE_ITEM (self), NULL);

  return self->action;
}

void
bs_page_item_set_action (BsPageItem *self,
                         const char   *action)
{
  g_return_if_fail (BS_IS_PAGE_ITEM (self));

  if (g_strcmp0 (self->action, action) == 0)
    return;

  g_clear_pointer (&self->action, g_free);
  self->action = g_strdup (action);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTION]);
}

BsPageItemType
bs_page_item_get_item_type (BsPageItem *self)
{
  g_return_val_if_fail (BS_IS_PAGE_ITEM (self), 0);

  return self->item_type;
}

void
bs_page_item_set_item_type (BsPageItem     *self,
                            BsPageItemType  item_type)
{
  g_return_if_fail (BS_IS_PAGE_ITEM (self));

  if (self->item_type == item_type)
    return;

  self->item_type = item_type;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TYPE]);
}

const char *
bs_page_item_get_factory (BsPageItem *self)
{
  g_return_val_if_fail (BS_IS_PAGE_ITEM (self), NULL);

  return self->factory;
}

void
bs_page_item_set_factory (BsPageItem *self,
                          const char   *factory)
{
  g_return_if_fail (BS_IS_PAGE_ITEM (self));

  if (g_strcmp0 (self->factory, factory) == 0)
    return;

  g_clear_pointer (&self->factory, g_free);
  self->factory = g_strdup (factory);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FACTORY]);
}

JsonNode *
bs_page_item_get_settings (BsPageItem *self)
{
  g_return_val_if_fail (BS_IS_PAGE_ITEM (self), NULL);

  return self->settings;
}

void
bs_page_item_set_settings (BsPageItem *self,
                           JsonNode     *settings)
{
  g_return_if_fail (BS_IS_PAGE_ITEM (self));
  g_return_if_fail (settings == NULL || JSON_NODE_HOLDS_OBJECT (settings));

  if (self->settings == settings)
    return;

  g_clear_pointer (&self->settings, json_node_unref);
  self->settings = settings ? json_node_ref (settings) : NULL;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SETTINGS]);
}

gboolean
bs_page_item_realize (BsPageItem  *self,
                      BsIcon       **out_custom_icon,
                      BsAction     **out_action,
                      GError       **error)
{
  const BsActionInfo *action_info;
  BsActionFactory *action_factory;

  g_return_val_if_fail (BS_IS_PAGE_ITEM (self), FALSE);
  g_return_val_if_fail (out_custom_icon != NULL, FALSE);
  g_return_val_if_fail (out_action != NULL, FALSE);

  switch (self->item_type)
    {
    case BS_PAGE_ITEM_EMPTY:
      *out_custom_icon = NULL; // TODO
      *out_action = bs_empty_action_new ();
      break;

    case BS_PAGE_ITEM_ACTION:
      action_factory = get_action_factory (self->factory);

      if (!action_factory)
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "No action factory named \"%s\"", self->factory);
          *out_custom_icon = NULL; // TODO
          *out_action = NULL;
          return FALSE;
        }

      action_info = bs_action_factory_get_info (action_factory, self->action);
      *out_custom_icon = NULL; // TODO
      *out_action = bs_action_factory_create_action (action_factory, action_info);
      if (self->settings)
        bs_action_deserialize_settings (*out_action, json_node_get_object (self->settings));
      break;
    }

  return TRUE;
}
