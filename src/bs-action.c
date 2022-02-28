/* bs-action.c
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
#include "bs-icon.h"

typedef struct
{
  char *id;
  char *name;
  BsActionFactory *factory;
  BsIcon *icon;
} BsActionPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BsAction, bs_action, G_TYPE_OBJECT)


/*
 * BsAction overrides
 */

static BsIcon *
bs_action_real_get_icon (BsAction *self)
{
  BsActionPrivate *priv = bs_action_get_instance_private (self);

  return priv->icon;
}

/*
 * GObject overrides
 */

static void
bs_action_finalize (GObject *object)
{
  BsAction *self = (BsAction *)object;
  BsActionPrivate *priv = bs_action_get_instance_private (self);

  g_clear_object (&priv->icon);
  g_clear_pointer (&priv->id, g_free);
  g_clear_pointer (&priv->name, g_free);

  G_OBJECT_CLASS (bs_action_parent_class)->finalize (object);
}

static void
bs_action_class_init (BsActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_action_finalize;

  klass->get_icon = bs_action_real_get_icon;
}

static void
bs_action_init (BsAction *self)
{
  BsActionPrivate *priv = bs_action_get_instance_private (self);

  priv->icon = bs_icon_new_empty ();
}

BsActionFactory *
bs_action_get_factory (BsAction *self)
{
  BsActionPrivate *priv;

  g_return_val_if_fail (BS_IS_ACTION (self), NULL);

  priv = bs_action_get_instance_private (self);
  return priv->factory;
}

void
bs_action_set_factory (BsAction        *self,
                       BsActionFactory *factory)
{

  BsActionPrivate *priv = bs_action_get_instance_private (self);

  g_return_if_fail (BS_IS_ACTION (self));
  g_return_if_fail (factory != NULL);
  g_return_if_fail (priv->factory == NULL);

  priv->factory = factory;
}

const char *
bs_action_get_id (BsAction *self)
{
  BsActionPrivate *priv;

  g_return_val_if_fail (BS_IS_ACTION (self), NULL);

  priv = bs_action_get_instance_private (self);
  return priv->id;
}

void
bs_action_set_id (BsAction   *self,
                  const char *id)
{
  BsActionPrivate *priv = bs_action_get_instance_private (self);

  g_return_if_fail (BS_IS_ACTION (self));
  g_return_if_fail (id != NULL);
  g_return_if_fail (priv->id == NULL);

  priv->id = g_strdup (id);
}

const char *
bs_action_get_name (BsAction *self)
{
  BsActionPrivate *priv;

  g_return_val_if_fail (BS_IS_ACTION (self), NULL);

  priv = bs_action_get_instance_private (self);
  return priv->name;
}

void
bs_action_set_name (BsAction   *self,
                    const char *name)
{
  BsActionPrivate *priv = bs_action_get_instance_private (self);

  g_return_if_fail (BS_IS_ACTION (self));
  g_return_if_fail (name != NULL);
  g_return_if_fail (priv->name == NULL);

  priv->name = g_strdup (name);
}


BsIcon *
bs_action_get_icon (BsAction *self)
{
  g_return_val_if_fail (BS_IS_ACTION (self), NULL);
  g_return_val_if_fail (BS_ACTION_GET_CLASS (self)->get_icon, NULL);

  return BS_ACTION_GET_CLASS (self)->get_icon (self);
}

void
bs_action_activate (BsAction *self)
{
  g_return_if_fail (BS_IS_ACTION (self));

  if (BS_ACTION_GET_CLASS (self)->activate)
    BS_ACTION_GET_CLASS (self)->activate (self);
}

void
bs_action_deactivate (BsAction *self)
{
  g_return_if_fail (BS_IS_ACTION (self));

  if (BS_ACTION_GET_CLASS (self)->deactivate)
    BS_ACTION_GET_CLASS (self)->deactivate (self);
}

GtkWidget *
bs_action_get_preferences (BsAction *self)
{
  g_return_val_if_fail (BS_IS_ACTION (self), NULL);

  if (BS_ACTION_GET_CLASS (self)->get_preferences)
    return BS_ACTION_GET_CLASS (self)->get_preferences (self);
  else
    return NULL;
}

JsonNode *
bs_action_serialize_settings (BsAction *self)
{
  g_return_val_if_fail (BS_IS_ACTION (self), NULL);

  if (BS_ACTION_GET_CLASS (self)->serialize_settings)
    {
      g_autoptr (JsonNode) node = NULL;

      node = BS_ACTION_GET_CLASS (self)->serialize_settings (self);

      if (!JSON_NODE_HOLDS_OBJECT (node))
        {
          g_warning ("Serialized action settings must be JsonObjects");
          return NULL;
        }

      return g_steal_pointer (&node);
    }
  else
    {
      return NULL;
    }
}

void
bs_action_deserialize_settings (BsAction   *self,
                                JsonObject *settings)
{
  g_return_if_fail (BS_IS_ACTION (self));
  g_return_if_fail (settings != NULL);

  if (BS_ACTION_GET_CLASS (self)->deserialize_settings)
    BS_ACTION_GET_CLASS (self)->deserialize_settings (self, settings);
}
