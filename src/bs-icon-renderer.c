/* bs-icon-renderer.c
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
#include "bs-icon-renderer.h"

struct _BsIconRenderer
{
  GObject parent_instance;

  const BsIconLayout *layout;
  GskRenderer *renderer;
};

G_DEFINE_FINAL_TYPE (BsIconRenderer, bs_icon_renderer, G_TYPE_OBJECT)

static void
bs_icon_renderer_finalize (GObject *object)
{
  BsIconRenderer *self = (BsIconRenderer *)object;

  g_clear_object (&self->renderer);

  G_OBJECT_CLASS (bs_icon_renderer_parent_class)->finalize (object);
}

static void
bs_icon_renderer_class_init (BsIconRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bs_icon_renderer_finalize;
}

static void
bs_icon_renderer_init (BsIconRenderer *self)
{
  self->renderer = gsk_cairo_renderer_new ();
}

BsIconRenderer *
bs_icon_renderer_new (const BsIconLayout *layout)
{
  BsIconRenderer *self;

  self = g_object_new (BS_TYPE_ICON_RENDERER, NULL);
  self->layout = layout;

  return self;
}

GdkTexture *
bs_icon_renderer_compose_icon (BsIconRenderer      *self,
                               BsIconComposeFlags   compose_flags,
                               BsIcon              *icon,
                               GError             **error)
{
  g_autoptr (GtkSnapshot) snapshot = NULL;
  g_autoptr (GskRenderNode) node = NULL;
  g_autoptr (GdkTexture) texture = NULL;

  g_return_val_if_fail (BS_IS_ICON_RENDERER (self), NULL);

  if (!gsk_renderer_realize (self->renderer, NULL, error))
    return NULL;

  snapshot = gtk_snapshot_new ();

  if (!(compose_flags & BS_ICON_COMPOSE_FLAG_IGNORE_TRANSFORMS))
    {
      gboolean flip_x = self->layout->flags & BS_ICON_RENDERER_FLAG_FLIP_X;
      gboolean flip_y = self->layout->flags & BS_ICON_RENDERER_FLAG_FLIP_Y;

      if (self->layout->flags & BS_ICON_RENDERER_FLAG_ROTATE_90)
        {
          gtk_snapshot_translate (snapshot,
                                  &GRAPHENE_POINT_INIT (self->layout->width / 2,
                                                        self->layout->height / 2));
          gtk_snapshot_rotate (snapshot, 90.0);
          gtk_snapshot_translate (snapshot,
                                  &GRAPHENE_POINT_INIT (- self->layout->width / 2,
                                                        - self->layout->height / 2));
        }

      gtk_snapshot_translate (snapshot,
                              &GRAPHENE_POINT_INIT (flip_x ? self->layout->width : 0,
                                                    flip_y ? self->layout->height : 0));
      gtk_snapshot_scale (snapshot,
                          flip_x ? -1.0 : 1.0,
                          flip_y ? -1.0 : 1.0);
    }

  if (icon)
    {
      bs_icon_snapshot_premultiplied (icon, snapshot, self->layout->width, self->layout->height);
    }
  else
    {
      gtk_snapshot_append_color (snapshot,
                                 &(GdkRGBA) { 0.0, 0.0, 0.0, 1.0, },
                                 &GRAPHENE_RECT_INIT (0, 0,
                                                      self->layout->width,
                                                      self->layout->height));
    }

  node = gtk_snapshot_free_to_node (g_steal_pointer (&snapshot));

  texture = gsk_renderer_render_texture (self->renderer,
                                         node,
                                         &GRAPHENE_RECT_INIT (0, 0, self->layout->width, self->layout->height));
  gsk_renderer_unrealize (self->renderer);

  return g_steal_pointer (&texture);
}

gboolean
bs_icon_renderer_convert_texture (BsIconRenderer  *self,
                                  GdkTexture      *texture,
                                  char           **buffer,
                                  size_t          *buffer_len,
                                  GError         **error)
{
  g_autoptr (GdkPixbuf) pixbuf = NULL;

  g_return_val_if_fail (BS_IS_ICON_RENDERER (self), FALSE);
  g_return_val_if_fail (GDK_IS_TEXTURE (texture), FALSE);
  g_return_val_if_fail (buffer != NULL, FALSE);
  g_return_val_if_fail (buffer_len != NULL, FALSE);

  pixbuf = gdk_pixbuf_get_from_texture (texture);

  switch (self->layout->format)
    {
    case BS_ICON_FORMAT_BMP:
      return gdk_pixbuf_save_to_buffer (pixbuf,
                                        buffer,
                                        buffer_len,
                                        "bmp",
                                        error,
                                        NULL);

    case BS_ICON_FORMAT_JPEG:
      return gdk_pixbuf_save_to_buffer (pixbuf,
                                        buffer,
                                        buffer_len,
                                        "jpeg",
                                        error,
                                        "quality", "96",
                                        NULL);

    default:
      g_assert_not_reached ();
    }
}
