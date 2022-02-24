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
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "bs-profile.h"

struct _BsProfile
{
  GObject parent_instance;
};

G_DEFINE_FINAL_TYPE (BsProfile, bs_profile, G_TYPE_OBJECT)

enum {
  PROP_0,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
bs_profile_finalize (GObject *object)
{
  BsProfile *self = (BsProfile *)object;

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
}

static void
bs_profile_init (BsProfile *self)
{
}

BsProfile *
bs_profile_new_from_key_file (const char  *file_path,
                              GError     **error)
{
  BsProfile *self;

  self = g_object_new (BS_TYPE_PROFILE, NULL);

  return self;
}
