/* bs-omni-bar.c
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "bs-omni-bar.h"

struct _BsOmniBar
{
  GtkWidget parent_instance;

  GtkBox *box;
  GtkMenuButton *menu_button;
  GtkBox *prefix;
  GtkBox *center;
  GtkBox *suffix;
  GtkPopover *popover;

  GBinding *menu_button_visibility_binding;
};

static void buildable_iface_init  (GtkBuildableIface      *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BsOmniBar, bs_omni_bar, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_iface_init))



enum
{
  PROP_0,
  PROP_POPOVER,
  PROP_MENU_POPOVER,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];


static void
bs_omni_bar_click_released_cb (BsOmniBar       *self,
                               int              n_relases,
                               double           x,
                               double           y,
                               GtkGestureClick *click)
{

  g_assert (BS_IS_OMNI_BAR (self));
  g_assert (GTK_IS_GESTURE_CLICK (click));

  if (self->popover == NULL)
    return;

  gtk_popover_popup (self->popover);
}


static void
bs_omni_bar_add_child (GtkBuildable *buildable,
                       GtkBuilder   *builder,
                       GObject      *child,
                       const char   *type)
{
  BsOmniBar *self = (BsOmniBar *)buildable;

  g_assert (GTK_IS_BUILDABLE (buildable));
  g_assert (GTK_IS_BUILDER (builder));
  g_assert (G_IS_OBJECT (child));

  if (!GTK_IS_WIDGET (child))
    {
      g_critical ("Attempted to add a non-widget to %s, which is not supported",
                  G_OBJECT_TYPE_NAME (self));
      return;
    }

  if (g_strcmp0 (type, "suffix") == 0)
    bs_omni_bar_add_suffix (self, 0, GTK_WIDGET (child));
  else
    bs_omni_bar_add_prefix (self, 0, GTK_WIDGET (child));
}

static void
buildable_iface_init (GtkBuildableIface *iface)
{
  iface->add_child = bs_omni_bar_add_child;
}

static void
bs_omni_bar_size_allocate (GtkWidget *widget,
                           int        width,
                           int        height,
                           int        baseline)
{
  BsOmniBar *self = (BsOmniBar *)widget;

  g_assert (BS_IS_OMNI_BAR (self));

  if (self->popover)
    gtk_popover_present (self->popover);
}

static void
bs_omni_bar_dispose (GObject *object)
{
  BsOmniBar *self = (BsOmniBar *)object;
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (bs_omni_bar_parent_class)->dispose (object);
}

static void
bs_omni_bar_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  BsOmniBar *self = BS_OMNI_BAR (object);

  switch (prop_id)
    {
    case PROP_POPOVER:
      g_value_set_object (value, bs_omni_bar_get_popover (self));
      break;

    case PROP_MENU_POPOVER:
      g_value_set_object (value, gtk_menu_button_get_popover (self->menu_button));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_omni_bar_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BsOmniBar *self = BS_OMNI_BAR (object);

  switch (prop_id)
    {
    case PROP_POPOVER:
      bs_omni_bar_set_popover (self, g_value_get_object (value));
      break;

    case PROP_MENU_POPOVER:
      gtk_menu_button_set_popover (self->menu_button, g_value_get_object (value));
      gtk_widget_set_visible (GTK_WIDGET (self->menu_button),
                              g_value_get_object (value) != NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_omni_bar_class_init (BsOmniBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = bs_omni_bar_dispose;
  object_class->get_property = bs_omni_bar_get_property;
  object_class->set_property = bs_omni_bar_set_property;

  widget_class->size_allocate = bs_omni_bar_size_allocate;

  properties [PROP_MENU_POPOVER] =
    g_param_spec_object ("menu-popover",
                         "Menu Model",
                         "Menu Model",
                         GTK_TYPE_POPOVER,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties [PROP_POPOVER] =
    g_param_spec_object ("popover",
                         "Popover",
                         "Popover",
                         GTK_TYPE_POPOVER,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "splitbutton");
}

static void
bs_omni_bar_init (BsOmniBar *self)
{
  GtkGesture *gesture;
  GtkWidget *separator;

  gtk_widget_add_css_class (GTK_WIDGET (self), "omnibar");

  self->box = g_object_new (GTK_TYPE_BOX,
                            "css-name", "button",
                            "orientation", GTK_ORIENTATION_HORIZONTAL,
                            NULL);
  gtk_widget_set_parent (GTK_WIDGET (self->box), GTK_WIDGET (self));

  separator = g_object_new (GTK_TYPE_SEPARATOR,
                            "orientation", GTK_ORIENTATION_VERTICAL,
                            NULL);
  gtk_widget_set_parent (separator, GTK_WIDGET (self));

  self->menu_button = g_object_new (GTK_TYPE_MENU_BUTTON,
                                    NULL);
  gtk_widget_set_parent (GTK_WIDGET (self->menu_button), GTK_WIDGET (self));
  g_object_bind_property (self->menu_button, "visible", separator, "visible", G_BINDING_DEFAULT);

  self->prefix = g_object_new (GTK_TYPE_BOX, NULL);
  self->center = g_object_new (GTK_TYPE_BOX,
                               "hexpand", TRUE,
                               NULL);
  self->suffix = g_object_new (GTK_TYPE_BOX, NULL);

  gtk_box_append (self->box, GTK_WIDGET (self->prefix));
  gtk_box_append (self->box, GTK_WIDGET (self->center));
  gtk_box_append (self->box, GTK_WIDGET (self->suffix));

  gesture = gtk_gesture_click_new ();
  g_signal_connect_object (gesture,
                           "released",
                           G_CALLBACK (bs_omni_bar_click_released_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_widget_add_controller (GTK_WIDGET (self->box), GTK_EVENT_CONTROLLER (gesture));
}

/**
 * bs_omni_bar_new:
 *
 * Create a new #BsOmniBar.
 *
 * Returns: (transfer full): a newly created #BsOmniBar
 */
GtkWidget *
bs_omni_bar_new (void)
{
  return g_object_new (BS_TYPE_OMNI_BAR, NULL);
}

/**
 * bs_omni_bar_get_popover:
 * @self: a #BsOmniBar
 *
 * Returns: (transfer none) (nullable): a #GtkPopover or %NULL
 */
GtkPopover *
bs_omni_bar_get_popover (BsOmniBar *self)
{

  g_return_val_if_fail (BS_IS_OMNI_BAR (self), NULL);

  return self->popover;
}

void
bs_omni_bar_set_popover (BsOmniBar  *self,
                         GtkPopover *popover)
{

  g_return_if_fail (BS_IS_OMNI_BAR (self));
  g_return_if_fail (!popover || GTK_IS_POPOVER (popover));

  if (popover == self->popover)
    return;

  if (self->popover)
    gtk_widget_unparent (GTK_WIDGET (self->popover));

  self->popover = popover;

  if (popover)
    gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_POPOVER]);
}

#define GET_PRIORITY(w)   GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"PRIORITY"))
#define SET_PRIORITY(w,i) g_object_set_data(G_OBJECT(w),"PRIORITY",GINT_TO_POINTER(i))

void
bs_omni_bar_add_prefix (BsOmniBar *self,
                        int        priority,
                        GtkWidget *widget)
{
  GtkWidget *sibling = NULL;

  g_return_if_fail (BS_IS_OMNI_BAR (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  SET_PRIORITY (widget, priority);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self->prefix));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (priority < GET_PRIORITY(child))
        break;
      sibling = child;
    }

  gtk_box_insert_child_after (self->prefix, widget, sibling);
}

void
bs_omni_bar_add_suffix (BsOmniBar *self,
                        int        priority,
                        GtkWidget *widget)
{
  GtkWidget *sibling = NULL;

  g_return_if_fail (BS_IS_OMNI_BAR (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  SET_PRIORITY (widget, priority);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self->suffix));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (priority < GET_PRIORITY(child))
        break;
      sibling = child;
    }

  gtk_box_insert_child_after (self->suffix, widget, sibling);
}

void
bs_omni_bar_remove (BsOmniBar *self,
                    GtkWidget *widget)
{
  GtkWidget *parent;

  g_return_if_fail (BS_IS_OMNI_BAR (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  parent = gtk_widget_get_parent (widget);

  if (parent == GTK_WIDGET (self->suffix) ||
      parent == GTK_WIDGET (self->prefix))
    {
      gtk_box_remove (GTK_BOX (parent), widget);
      return;
    }

  /* TODO: Support removing internal things */
}
