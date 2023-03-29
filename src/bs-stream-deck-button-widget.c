/*
 * bs-stream-deck-button-widget.c
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

#define G_LOG_DOMAIN "Stream Deck Button Widget"

#include "bs-stream-deck-button-widget.h"

#include "bs-action-private.h"
#include "bs-debug.h"
#include "bs-empty-action.h"
#include "bs-icon.h"
#include "bs-page.h"
#include "bs-stream-deck.h"
#include "bs-stream-deck-button.h"

#include <libpeas.h>

struct _BsStreamDeckButtonWidget
{
  GtkFlowBoxChild parent_instance;

  GtkDragSource *drag_source;
  GtkDropTarget *drop_target;
  GtkPicture *picture;

  BsStreamDeckButton *button;
  int icon_size;
};

G_DEFINE_FINAL_TYPE (BsStreamDeckButtonWidget, bs_stream_deck_button_widget, GTK_TYPE_FLOW_BOX_CHILD)

enum
{
  PROP_0,
  PROP_BUTTON,
  PROP_ICON_SIZE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/*
 * Auxiliary methods
 */

static inline gboolean
is_move_page_up_action (BsStreamDeckButtonWidget *self)
{
  const PeasPluginInfo *plugin_info;
  BsActionFactory *factory;
  BsStreamDeck *stream_deck;
  BsAction *action;
  BsPage *active_page;

  action = bs_stream_deck_button_get_action (self->button);
  if (!action || BS_IS_EMPTY_ACTION (action))
    return FALSE;

  factory = bs_action_get_factory (action);
  plugin_info = peas_extension_base_get_plugin_info (PEAS_EXTENSION_BASE (factory));
  stream_deck = bs_stream_deck_button_get_stream_deck (self->button);
  active_page = bs_stream_deck_get_active_page (stream_deck);

  return bs_stream_deck_button_get_position (self->button) == 0 &&
         bs_page_get_parent (active_page) != NULL &&
         g_strcmp0 (peas_plugin_info_get_module_name (plugin_info), "default") == 0 &&
         g_strcmp0 (bs_action_get_id (action), "default-switch-page-action") == 0;
}

static void
select_myself_in_parent_flowbox (BsStreamDeckButtonWidget *self)
{
  GtkWidget *flowbox;

  flowbox = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_FLOW_BOX);
  gtk_flow_box_select_child (GTK_FLOW_BOX (flowbox), GTK_FLOW_BOX_CHILD (self));
}

static void
update_button_texture (BsStreamDeckButtonWidget *self)
{
  BsIcon *icon = bs_stream_deck_button_get_icon (self->button);
  gtk_picture_set_paintable (self->picture, GDK_PAINTABLE (icon));
}


/*
 * Callbacks
 */

static GdkContentProvider *
on_drag_prepare_cb (GtkDragSource            *drag_source,
                    double                    x,
                    double                    y,
                    BsStreamDeckButtonWidget *self)
{
  BS_RETURN (gdk_content_provider_new_typed (BS_TYPE_STREAM_DECK_BUTTON, self->button));
}

static void
on_drag_begin_cb (GtkDragSource            *drag_source,
                  GdkDrag                  *drag,
                  BsStreamDeckButtonWidget *self)
{
  BsIcon *icon;

  BS_ENTRY;

  icon = bs_stream_deck_button_get_icon (self->button);
  gtk_drag_source_set_icon (drag_source, GDK_PAINTABLE (icon), 0, 0);

  BS_EXIT;
}

static gboolean
on_drop_target_drop_cb (GtkDropTarget            *drop_target,
                        const GValue             *value,
                        double                    x,
                        double                    y,
                        BsStreamDeckButtonWidget *self)
{
  g_autoptr (BsAction) dragged_button_action = NULL;
  g_autoptr (BsAction) dropped_button_action = NULL;
  BsStreamDeckButton *dragged_stream_deck_button;
  BsStreamDeckButton *dropped_stream_deck_button;
  g_autoptr (BsIcon) dragged_button_icon = NULL;
  g_autoptr (BsIcon) dropped_button_icon = NULL;

  BS_ENTRY;

  if (!G_VALUE_HOLDS (value, BS_TYPE_STREAM_DECK_BUTTON))
    BS_RETURN (FALSE);

  dropped_stream_deck_button = self->button;
  dragged_stream_deck_button = g_value_get_object (value);

  if (dragged_stream_deck_button == dropped_stream_deck_button)
    BS_RETURN (FALSE);

  /* Swap actions */
  dragged_button_action = bs_stream_deck_button_get_action (dragged_stream_deck_button);
  dropped_button_action = bs_stream_deck_button_get_action (dropped_stream_deck_button);
  dragged_button_icon = bs_stream_deck_button_get_custom_icon (dragged_stream_deck_button);
  dropped_button_icon = bs_stream_deck_button_get_custom_icon (dropped_stream_deck_button);

  if (dragged_button_action)
    g_object_ref (dragged_button_action);
  if (dropped_button_action)
    g_object_ref (dropped_button_action);
  if (dragged_button_icon)
    g_object_ref (dragged_button_icon);
  if (dropped_button_icon)
    g_object_ref (dropped_button_icon);

  bs_stream_deck_button_set_action (dragged_stream_deck_button, dropped_button_action);
  bs_stream_deck_button_set_action (dropped_stream_deck_button, dragged_button_action);

  bs_stream_deck_button_set_custom_icon (dragged_stream_deck_button, dropped_button_icon);
  bs_stream_deck_button_set_custom_icon (dropped_stream_deck_button, dragged_button_icon);

  select_myself_in_parent_flowbox (self);

  BS_RETURN (TRUE);
}

static void
on_stream_deck_button_action_changed_cb (BsStreamDeckButton       *stream_deck_button,
                                         GParamSpec               *pspec,
                                         BsStreamDeckButtonWidget *self)
{
  if (is_move_page_up_action (self))
    {
      gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (self->drag_source),
                                                  GTK_PHASE_NONE);
      gtk_drop_target_set_actions (self->drop_target, 0);
    }
  else
    {
      gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (self->drag_source),
                                                  GTK_PHASE_BUBBLE);
      gtk_drop_target_set_actions (self->drop_target, GDK_ACTION_MOVE);
    }
}

static void
on_stream_deck_button_icon_changed_cb (BsStreamDeckButton       *stream_deck_button,
                                       BsIcon                   *icon,
                                       BsStreamDeckButtonWidget *self)
{
  update_button_texture (self);
}


/*
 * Overrides
 */

static void
bs_stream_deck_button_widget_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  BsStreamDeckButtonWidget *self = BS_STREAM_DECK_BUTTON_WIDGET (object);

  switch (prop_id)
    {
    case PROP_BUTTON:
      g_value_set_object (value, self->button);
      break;

    case PROP_ICON_SIZE:
      g_value_set_int (value, self->icon_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_button_widget_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  BsStreamDeckButtonWidget *self = BS_STREAM_DECK_BUTTON_WIDGET (object);

  switch (prop_id)
    {
    case PROP_BUTTON:
      g_assert (self->button == NULL);
      self->button = g_value_get_object (value);
      g_signal_connect_object (self->button,
                               "notify::action",
                               G_CALLBACK (on_stream_deck_button_action_changed_cb),
                               self,
                               0);
      g_signal_connect_object (self->button,
                               "icon-changed",
                               G_CALLBACK (on_stream_deck_button_icon_changed_cb),
                               self,
                               0);
      update_button_texture (self);
      break;

    case PROP_ICON_SIZE:
      self->icon_size = g_value_get_int (value);
      g_object_notify_by_pspec (object, properties[PROP_ICON_SIZE]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_button_widget_class_init (BsStreamDeckButtonWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = bs_stream_deck_button_widget_get_property;
  object_class->set_property = bs_stream_deck_button_widget_set_property;

  properties[PROP_BUTTON] = g_param_spec_object ("button", NULL, NULL,
                                                 BS_TYPE_STREAM_DECK_BUTTON,
                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_ICON_SIZE] = g_param_spec_int ("icon-size", NULL, NULL,
                                                 -1, G_MAXINT, -1,
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/bs-stream-deck-button-widget.ui");

  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonWidget, drag_source);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckButtonWidget, picture);

  gtk_widget_class_bind_template_callback (widget_class, on_drag_prepare_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_drag_begin_cb);

  gtk_widget_class_set_css_name (widget_class, "streamdeckbuttonwidget");
}

static void
bs_stream_deck_button_widget_init (BsStreamDeckButtonWidget *self)
{
  self->icon_size = -1;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->drop_target = gtk_drop_target_new (BS_TYPE_STREAM_DECK_BUTTON, GDK_ACTION_MOVE);
  gtk_drop_target_set_preload (self->drop_target, TRUE);
  g_signal_connect (self->drop_target, "drop", G_CALLBACK (on_drop_target_drop_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drop_target));
}

GtkWidget *
bs_stream_deck_button_widget_new (BsStreamDeckButton *button,
                                  int                 icon_size)
{
  return g_object_new (BS_TYPE_STREAM_DECK_BUTTON_WIDGET,
                       "button", button,
                       "icon-size", icon_size,
                       NULL);
}
