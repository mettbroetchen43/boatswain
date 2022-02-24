/* bs-icon.c
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

#include "bs-icon.h"

struct _BsIcon
{
  GObject parent_instance;

  GdkRGBA background_color;
  GdkPaintable *paintable;
  PangoLayout *layout;
  int margin;

  gulong size_changed_id;
  gulong content_changed_id;
};

static void gdk_paintable_iface_init (GdkPaintableInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BsIcon, bs_icon, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE, gdk_paintable_iface_init))

enum
{
  PROP_0,
  PROP_BACKGROUND_COLOR,
  PROP_MARGIN,
  PROP_PAINTABLE,
  PROP_TEXT,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/*
 * Callbacks
 */

static void
on_paintable_contents_changed_cb (GdkPaintable *paintable,
                                  BsIcon       *self)
{
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
}

static void
on_paintable_size_changed_cb (GdkPaintable *paintable,
                              BsIcon       *self)
{
  gdk_paintable_invalidate_size (GDK_PAINTABLE (self));
}


/*
 * GdkPaintable interface
 */

static double
bs_icon_get_intrinsic_aspect_ratio (GdkPaintable *paintable)
{
  BsIcon *self = BS_ICON (paintable);

  return self->paintable ? gdk_paintable_get_intrinsic_aspect_ratio (self->paintable) : 1.0;
}

static int
bs_icon_get_intrinsic_width (GdkPaintable *paintable)
{
  BsIcon *self = BS_ICON (paintable);

  return self->paintable ? gdk_paintable_get_intrinsic_width (self->paintable) : 0;
}


static int
bs_icon_get_intrinsic_height (GdkPaintable *paintable)
{
  BsIcon *self = BS_ICON (paintable);

  return self->paintable ? gdk_paintable_get_intrinsic_height (self->paintable) : 0;
}

static void
bs_icon_snapshot (GdkPaintable *paintable,
                  GdkSnapshot  *snapshot,
                  double        width,
                  double        height)
{
  BsIcon *self = BS_ICON (paintable);

  gtk_snapshot_append_color (snapshot,
                             &self->background_color,
                             &GRAPHENE_RECT_INIT (0, 0, width, height));

  if (self->paintable)
    {
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (self->margin, self->margin));

      g_message ("Painting (%d)", self->margin);

      gdk_paintable_snapshot (self->paintable,
                              snapshot,
                              width - self->margin * 2,
                              height - self->margin * 2);

      gtk_snapshot_restore (snapshot);
    }

  if (self->layout)
    {
      int text_width, text_height;
      int x, y;

      pango_layout_get_pixel_size (self->layout, &text_width, &text_height);
      x = (width - text_width) / 2.0;
      y = height - text_height;

      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
      gtk_snapshot_append_layout (snapshot, self->layout, &(GdkRGBA) { 1.0, 1.0, 1.0, 1.0 });
      gtk_snapshot_restore (snapshot);
    }
}

static void
gdk_paintable_iface_init (GdkPaintableInterface *iface)
{
  iface->get_intrinsic_aspect_ratio = bs_icon_get_intrinsic_aspect_ratio;
  iface->get_intrinsic_height = bs_icon_get_intrinsic_height;
  iface->get_intrinsic_width = bs_icon_get_intrinsic_width;
  iface->snapshot = bs_icon_snapshot;
}


/*
 * GObject overrides
 */

static void
bs_icon_finalize (GObject *object)
{
  BsIcon *self = (BsIcon *)object;

  g_clear_signal_handler (&self->content_changed_id, self->paintable);
  g_clear_signal_handler (&self->size_changed_id, self->paintable);
  g_clear_object (&self->paintable);
  g_clear_object (&self->layout);

  G_OBJECT_CLASS (bs_icon_parent_class)->finalize (object);
}

static void
bs_icon_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  BsIcon *self = BS_ICON (object);

  switch (prop_id)
    {
    case PROP_BACKGROUND_COLOR:
      g_value_set_boxed (value, &self->background_color);
      break;

    case PROP_MARGIN:
      g_value_set_int (value, self->margin);
      break;

    case PROP_PAINTABLE:
      g_value_set_object (value, self->paintable);
      break;

    case PROP_TEXT:
      g_value_set_string (value, self->layout ? pango_layout_get_text (self->layout) : NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_icon_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  BsIcon *self = BS_ICON (object);

  switch (prop_id)
    {
    case PROP_BACKGROUND_COLOR:
      bs_icon_set_background_color (self, g_value_get_boxed (value));
      break;

    case PROP_MARGIN:
      bs_icon_set_margin (self, g_value_get_int (value));
      break;

    case PROP_PAINTABLE:
      bs_icon_set_paintable (self, g_value_get_object (value));
      break;

    case PROP_TEXT:
      bs_icon_set_text (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_icon_class_init (BsIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_icon_finalize;
  object_class->get_property = bs_icon_get_property;
  object_class->set_property = bs_icon_set_property;

  properties[PROP_BACKGROUND_COLOR] = g_param_spec_boxed ("background-color", NULL, NULL,
                                                          GDK_TYPE_RGBA,
                                                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_MARGIN] = g_param_spec_int ("margin", NULL, NULL,
                                              0, G_MAXINT, 12,
                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_PAINTABLE] = g_param_spec_object ("paintable", NULL, NULL,
                                                    GDK_TYPE_PAINTABLE,
                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_TEXT] = g_param_spec_string ("text", NULL, NULL,
                                               NULL,
                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bs_icon_init (BsIcon *self)
{
  self->margin = 12;
}


BsIcon *
bs_icon_new_empty (void)
{
  GdkRGBA transparent = (GdkRGBA) { 0.0, 0.0, 0.0, 0.0 };

  return bs_icon_new (&transparent, NULL);
}

BsIcon *
bs_icon_new (const GdkRGBA *background_color,
             GdkPaintable  *paintable)
{
  return g_object_new (BS_TYPE_ICON,
                       "background-color", background_color,
                       "paintable", paintable,
                       NULL);
}

const GdkRGBA *
bs_icon_get_background_color (BsIcon *self)
{
  g_return_val_if_fail (BS_IS_ICON (self), NULL);

  return &self->background_color;
}

void
bs_icon_set_background_color (BsIcon        *self,
                              const GdkRGBA *background_color)
{
  g_return_if_fail (BS_IS_ICON (self));

  if (background_color)
    self->background_color = *background_color;
  else
    self->background_color = (GdkRGBA) { 0.0, 0.0, 0.0, 0.0 };

  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BACKGROUND_COLOR]);
}

GdkPaintable *
bs_icon_get_paintable (BsIcon *self)
{
  g_return_val_if_fail (BS_IS_ICON (self), NULL);

  return self->paintable;
}

void
bs_icon_set_paintable (BsIcon       *self,
                       GdkPaintable *paintable)
{
  g_return_if_fail (BS_IS_ICON (self));
  g_return_if_fail (paintable != GDK_PAINTABLE (self));

  if (self->paintable == paintable)
    return;

  g_clear_signal_handler (&self->content_changed_id, self->paintable);
  g_clear_signal_handler (&self->size_changed_id, self->paintable);

  g_set_object (&self->paintable, paintable);

  self->content_changed_id = g_signal_connect (paintable,
                                               "invalidate-contents",
                                               G_CALLBACK (on_paintable_contents_changed_cb),
                                               self);

  self->size_changed_id = g_signal_connect (paintable,
                                            "invalidate-contents",
                                            G_CALLBACK (on_paintable_size_changed_cb),
                                            self);

  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
  gdk_paintable_invalidate_size (GDK_PAINTABLE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PAINTABLE]);
}

const char *
bs_icon_get_text (BsIcon *self)
{
  g_return_val_if_fail (BS_IS_ICON (self), NULL);

  return self->layout ? pango_layout_get_text (self->layout) : NULL;
}

void
bs_icon_set_text (BsIcon     *self,
                  const char *text)
{
  g_return_if_fail (BS_IS_ICON (self));

  if (self->layout && g_strcmp0 (pango_layout_get_text (self->layout), text) == 0)
    return;

  if (text)
    {
      if (!self->layout)
        {
          g_autoptr (PangoFontDescription) font_description = NULL;
          PangoContext *pango_context;

          font_description = pango_font_description_from_string ("Cantarell 10");

          pango_context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
          pango_context_set_language (pango_context, gtk_get_default_language ());
          pango_context_set_font_description (pango_context, font_description);

          self->layout = pango_layout_new (pango_context);

          g_clear_object (&pango_context);
        }

      pango_layout_set_text (self->layout, text, -1);
    }
  else
    {
      g_clear_object (&self->layout);
    }

  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
  gdk_paintable_invalidate_size (GDK_PAINTABLE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TEXT]);
}

int
bs_icon_get_margin (BsIcon *self)
{
  g_return_val_if_fail (BS_IS_ICON (self), 0);

  return self->margin;
}

void
bs_icon_set_margin (BsIcon *self,
                    int     margin)
{
  g_return_if_fail (BS_IS_ICON (self));

  if (self->margin == margin)
    return;

  self->margin = margin;

  g_message ("Setting to %d", margin);

  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
  gdk_paintable_invalidate_size (GDK_PAINTABLE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MARGIN]);
}
