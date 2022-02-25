/* bs-profile.c
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
 * SPDX-License-Identifier: GPL-3.0-or-laterfinalize
 */

#include "bs-profile.h"
#include "bs-page.h"

#include <glib/gi18n.h>

struct _BsProfile
{
  GObject parent_instance;

  char *id;
  char *name;
  double brightness;
  BsPage *root_page;
};

static void json_serializable_iface_init (JsonSerializableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BsProfile, bs_profile, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE, json_serializable_iface_init))

enum
{
  PROP_0,
  PROP_ID,
  PROP_NAME,
  PROP_BRIGHTNESS,
  PROP_PAGE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

/*
 * JsonSerializable interface
 */

static gboolean
bs_profile_deserialize_property (JsonSerializable *serializable,
                                 const char       *property_name,
                                 GValue           *value,
                                 GParamSpec       *pspec,
                                 JsonNode         *property_node)
{
  if (g_strcmp0 (property_name, "page") == 0)
    {
      g_value_set_object (value, bs_page_new_from_json (NULL, property_node));
      return TRUE;
    }

  return json_serializable_default_deserialize_property (serializable,
                                                         property_name,
                                                         value,
                                                         pspec,
                                                         property_node);
}

static JsonNode *
bs_profile_serialize_property (JsonSerializable *serializable,
                               const char       *property_name,
                               const GValue     *value,
                               GParamSpec       *pspec)
{
  BsProfile *self = BS_PROFILE (serializable);

  if (g_strcmp0 (property_name, "page") == 0)
    return bs_page_to_json (self->root_page);

  return json_serializable_default_serialize_property (serializable,
                                                       property_name,
                                                       value,
                                                       pspec);
}
static void
json_serializable_iface_init (JsonSerializableIface *iface)
{
  iface->serialize_property = bs_profile_serialize_property;
  iface->deserialize_property = bs_profile_deserialize_property;
}


/*
 * GObject overrides
 */

static void
bs_profile_finalize (GObject *object)
{
  BsProfile *self = (BsProfile *)object;

  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->name, g_free);

  G_OBJECT_CLASS (bs_profile_parent_class)->finalize (object);
}

static void
bs_profile_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  BsProfile *self = BS_PROFILE (object);

  switch (prop_id)
    {
    case PROP_BRIGHTNESS:
      g_value_set_double (value, self->brightness);
      break;

    case PROP_ID:
      g_value_set_string (value, self->id);
      break;

    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_PAGE:
      g_value_set_object (value, self->root_page);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_profile_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  BsProfile *self = BS_PROFILE (object);

  switch (prop_id)
    {
    case PROP_BRIGHTNESS:
      bs_profile_set_brightness (self, g_value_get_double (value));
      break;

    case PROP_ID:
      g_assert (self->id == NULL);
      self->id = g_value_dup_string (value);
      break;

    case PROP_NAME:
      bs_profile_set_name (self, g_value_get_string (value));
      break;

    case PROP_PAGE:
      g_assert (self->root_page == NULL);
      self->root_page = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_profile_class_init (BsProfileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_profile_finalize;
  object_class->get_property = bs_profile_get_property;
  object_class->set_property = bs_profile_set_property;

  properties[PROP_BRIGHTNESS] = g_param_spec_double ("brightness", NULL, NULL,
                                                     0.0, 1.0, 0.5,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_ID] = g_param_spec_string ("id", NULL, NULL,
                                             NULL,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_NAME] = g_param_spec_string ("name", NULL, NULL,
                                               NULL,
                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_PAGE] = g_param_spec_object ("page", NULL, NULL,
                                               BS_TYPE_PAGE,
                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_profile_init (BsProfile *self)
{
  self->brightness = 0.5;
}

BsProfile *
bs_profile_new_empty (void)
{
  g_autofree char *id = g_uuid_string_random ();

  return g_object_new (BS_TYPE_PROFILE,
                       "id", id,
                       "name", _("Unnamed profile"),
                       "page", bs_page_new_empty (NULL),
                       NULL);

}

double
bs_profile_get_brightness (BsProfile *self)
{
  g_return_val_if_fail (BS_IS_PROFILE (self), 0.0);

  return self->brightness;
}

void
bs_profile_set_brightness (BsProfile *self,
                           double     brightness)
{
  g_return_if_fail (BS_IS_PROFILE (self));
  g_return_if_fail (brightness >= 0.0 && brightness <= 1.0);

  if (G_APPROX_VALUE (self->brightness, brightness, DBL_EPSILON))
    return;

  self->brightness = brightness;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BRIGHTNESS]);
}

const char *
bs_profile_get_name (BsProfile *self)
{
  g_return_val_if_fail (BS_IS_PROFILE (self), NULL);

  return self->name;
}

const char *
bs_profile_get_id (BsProfile *self)
{
  g_return_val_if_fail (BS_IS_PROFILE (self), NULL);

  return self->id;
}

void
bs_profile_set_name (BsProfile  *self,
                     const char *name)
{
  g_return_if_fail (BS_IS_PROFILE (self));
  g_return_if_fail (name != NULL && *name != '\0');

  if (g_strcmp0 (self->name, name) == 0)
    return;

  g_clear_pointer (&self->name, g_free);
  self->name = g_strdup (name);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_NAME]);
}

BsPage *
bs_profile_get_root_page (BsProfile *self)
{
  g_return_val_if_fail (BS_IS_PROFILE (self), NULL);

  return self->root_page;
}
