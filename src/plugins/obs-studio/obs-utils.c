/* obs-utils.c
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

#include "obs-utils.h"

static ObsSourceType
guess_source_type_from_caps (ObsSourceCaps caps)
{
  if (caps & OBS_SOURCE_CAP_VIDEO)
    return OBS_SOURCE_TYPE_VIDEO;

  if (caps & OBS_SOURCE_CAP_AUDIO)
    return OBS_SOURCE_TYPE_AUDIO;

  return OBS_SOURCE_TYPE_UNKNOWN;
}

static ObsSourceType
parse_filter_cb (const char    *identifier,
                 ObsSourceCaps  caps)
{
  return OBS_SOURCE_TYPE_FILTER;
}

static ObsSourceType
parse_input_cb (const char    *identifier,
                ObsSourceCaps  caps)
{
  const struct {
    const char *identifier;
    ObsSourceType source_type;
  } hardcoded_source_types[] = {
    { "jack_output_capture", OBS_SOURCE_TYPE_AUDIO },
    { "pulse_input_capture", OBS_SOURCE_TYPE_MICROPHONE },
    { "pulse_output_capture", OBS_SOURCE_TYPE_AUDIO },
  };

  for (size_t i = 0; i < G_N_ELEMENTS (hardcoded_source_types); i++)
    {
      if (g_strcmp0 (hardcoded_source_types[i].identifier, identifier) == 0)
        return hardcoded_source_types[i].source_type;
    }

  return guess_source_type_from_caps (caps);
}

static ObsSourceType
parse_other_cb (const char    *identifier,
                ObsSourceCaps  caps)
{
  return guess_source_type_from_caps (caps);
}

static ObsSourceType
parse_transition_cb (const char    *identifier,
                     ObsSourceCaps  caps)
{
  return OBS_SOURCE_TYPE_TRANSITION;
}

static const struct {
  const char *type;
  ObsSourceType (*parse_func) (const char    *identifier,
                               ObsSourceCaps  caps);
} source_parse_vtable[] = {
  { "filter", parse_filter_cb },
  { "input", parse_input_cb },
  { "other", parse_other_cb },
  { "transition", parse_transition_cb },
};

ObsSourceType
obs_parse_source_type (const char    *type,
                       const char    *identifier,
                       ObsSourceCaps  caps)
{
  for (size_t i = 0; i < G_N_ELEMENTS (source_parse_vtable); i++)
    {
      if (g_strcmp0 (source_parse_vtable[i].type, type) == 0)
        return source_parse_vtable[i].parse_func (identifier, caps);
    }

  return guess_source_type_from_caps (caps);
}
