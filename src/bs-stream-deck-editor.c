/* bs-stream-deck-editor.c
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

#define G_LOG_DOMAIN "Stream Deck Editor"

#include "bs-action-private.h"
#include "bs-debug.h"
#include "bs-icon.h"
#include "bs-icon-renderer.h"
#include "bs-page.h"
#include "bs-stream-deck.h"
#include "bs-stream-deck-button.h"
#include "bs-stream-deck-button-editor.h"
#include "bs-stream-deck-editor.h"

#include <glib/gi18n.h>
#include <libpeas/peas.h>

struct _BsStreamDeckEditor
{
  AdwBin parent_instance;

  BsStreamDeckButtonEditor *button_editor;
  GtkFlowBox *buttons_flowbox;

  GtkFlowBoxChild *active_button;

  BsStreamDeck *stream_deck;
};

static void on_stream_deck_button_icon_changed_cb (BsStreamDeckButton *stream_deck_button,
                                                   BsIcon             *icon,
                                                   BsStreamDeckEditor *self);

G_DEFINE_FINAL_TYPE (BsStreamDeckEditor, bs_stream_deck_editor, ADW_TYPE_BIN)

enum
{
  PROP_0,
  PROP_STREAM_DECK,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/*
 * Auxiliary methods
 */

static inline gboolean
is_switch_page_action (BsAction *action)
{
  const PeasPluginInfo *plugin_info;
  BsActionFactory *factory;

  factory = bs_action_get_factory (action);
  plugin_info = peas_extension_base_get_plugin_info (PEAS_EXTENSION_BASE (factory));

  return g_strcmp0 (peas_plugin_info_get_module_name (plugin_info), "default") == 0 &&
         g_strcmp0 (bs_action_get_id (action), "default-switch-page-action") == 0;
}

static void
update_button_texture (BsStreamDeckEditor *self,
                       GtkFlowBoxChild    *child)
{
  BsStreamDeckButton *stream_deck_button;
  GtkWidget *picture;
  BsIcon *icon;
  int position;

  position = gtk_flow_box_child_get_index (child);
  stream_deck_button = bs_stream_deck_get_button (self->stream_deck, position);
  icon = bs_stream_deck_button_get_icon (stream_deck_button);

  picture = gtk_flow_box_child_get_child (child);
  gtk_picture_set_paintable (GTK_PICTURE (picture), GDK_PAINTABLE (icon));
}

static GdkContentProvider *
on_drag_prepare_cb (GtkDragSource      *drag_source,
                    double              x,
                    double              y,
                    BsStreamDeckEditor *self)
{
  BsStreamDeckButton *stream_deck_button;
  GtkWidget *flowbox_child;
  int position;

  BS_ENTRY;

  flowbox_child = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (drag_source));
  position = gtk_flow_box_child_get_index (GTK_FLOW_BOX_CHILD (flowbox_child));
  stream_deck_button = bs_stream_deck_get_button (self->stream_deck, position);

  BS_RETURN (gdk_content_provider_new_typed (BS_TYPE_STREAM_DECK_BUTTON, stream_deck_button));
}

static void
on_drag_begin_cb (GtkDragSource      *drag_source,
                  GdkDrag            *drag,
                  BsStreamDeckEditor *self)
{
  BsStreamDeckButton *stream_deck_button;
  GtkWidget *flowbox_child;
  BsIcon *icon;
  int position;

  BS_ENTRY;

  flowbox_child = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (drag_source));
  position = gtk_flow_box_child_get_index (GTK_FLOW_BOX_CHILD (flowbox_child));
  stream_deck_button = bs_stream_deck_get_button (self->stream_deck, position);
  icon = bs_stream_deck_button_get_icon (stream_deck_button);

  gtk_drag_source_set_icon (drag_source, GDK_PAINTABLE (icon), 0, 0);

  BS_EXIT;
}

static gboolean
on_drop_target_drop_cb (GtkDropTarget      *drop_target,
                        const GValue       *value,
                        double              x,
                        double              y,
                        BsStreamDeckEditor *self)
{
  g_autoptr (BsAction) dragged_button_action = NULL;
  g_autoptr (BsAction) dropped_button_action = NULL;
  BsStreamDeckButton *dragged_stream_deck_button;
  BsStreamDeckButton *dropped_stream_deck_button;
  g_autoptr (BsIcon) dragged_button_icon = NULL;
  g_autoptr (BsIcon) dropped_button_icon = NULL;
  GtkWidget *flowbox_child;
  int position;

  BS_ENTRY;

  if (!G_VALUE_HOLDS (value, BS_TYPE_STREAM_DECK_BUTTON))
    BS_RETURN (FALSE);

  flowbox_child = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (drop_target));
  position = gtk_flow_box_child_get_index (GTK_FLOW_BOX_CHILD (flowbox_child));
  dropped_stream_deck_button = bs_stream_deck_get_button (self->stream_deck, position);
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

  gtk_widget_activate (flowbox_child);

  BS_RETURN (TRUE);
}

static void
build_button_grid (BsStreamDeckEditor *self)
{
  const BsStreamDeckButtonLayout *layout;
  uint8_t i;

  layout = bs_stream_deck_get_button_layout (self->stream_deck);
  gtk_flow_box_set_max_children_per_line (self->buttons_flowbox, layout->columns);
  gtk_flow_box_set_min_children_per_line (self->buttons_flowbox, layout->columns);

  for (i = 0; i < layout->n_buttons; i++)
    {
      BsStreamDeckButton *stream_deck_button;
      GtkDragSource *drag_source;
      GtkDropTarget *drop_target;
      GtkWidget *button;
      GtkWidget *picture;

      button = gtk_flow_box_child_new ();
      gtk_flow_box_append (self->buttons_flowbox, button);

      picture = gtk_picture_new ();
      gtk_widget_add_css_class (picture, "card");
      gtk_widget_set_size_request (picture, layout->icon_size, layout->icon_size);
      gtk_widget_set_halign (picture, GTK_ALIGN_CENTER);
      gtk_widget_set_overflow (picture, GTK_OVERFLOW_HIDDEN);
      gtk_flow_box_child_set_child (GTK_FLOW_BOX_CHILD (button), picture);

      stream_deck_button = bs_stream_deck_get_button (self->stream_deck, i);
      g_signal_connect_object (stream_deck_button,
                               "icon-changed",
                               G_CALLBACK (on_stream_deck_button_icon_changed_cb),
                               self,
                               0);

      update_button_texture (self, GTK_FLOW_BOX_CHILD (button));

      drag_source = gtk_drag_source_new ();
      gtk_drag_source_set_actions (drag_source, GDK_ACTION_MOVE);
      g_signal_connect (drag_source, "prepare", G_CALLBACK (on_drag_prepare_cb), self);
      g_signal_connect (drag_source, "drag-begin", G_CALLBACK (on_drag_begin_cb), self);
      gtk_widget_add_controller (GTK_WIDGET (button), GTK_EVENT_CONTROLLER (drag_source));

      drop_target = gtk_drop_target_new (BS_TYPE_STREAM_DECK_BUTTON, GDK_ACTION_MOVE);
      gtk_drop_target_set_preload (drop_target, TRUE);
      g_signal_connect (drop_target, "drop", G_CALLBACK (on_drop_target_drop_cb), self);
      gtk_widget_add_controller (GTK_WIDGET (button), GTK_EVENT_CONTROLLER (drop_target));
    }
}


/*
 * Callbacks
 */

static void
on_flowbox_child_activated_cb (GtkFlowBox         *flowbox,
                               GtkFlowBoxChild    *child,
                               BsStreamDeckEditor *self)
{
  BsStreamDeckButton *button;
  BsAction *action;
  int position;

  position = gtk_flow_box_child_get_index (child);
  button = bs_stream_deck_get_button (self->stream_deck, position);
  action = bs_stream_deck_button_get_action (button);

  if (action && is_switch_page_action (action))
    bs_action_activate (action);
}

static void
on_flowbox_selected_children_changed_cb (GtkFlowBox         *flowbox,
                                         BsStreamDeckEditor *self)
{
  g_autoptr (GList) selected_children = NULL;
  GtkFlowBoxChild *child;

  selected_children = gtk_flow_box_get_selected_children (flowbox);
  child = selected_children ? selected_children->data : NULL;

  self->active_button = child;

  if (child)
    {
      BsStreamDeckButton *stream_deck_button;
      int position;

      position = gtk_flow_box_child_get_index (child);
      stream_deck_button = bs_stream_deck_get_button (self->stream_deck, position);

      bs_stream_deck_button_editor_set_button (self->button_editor, stream_deck_button);
    }
}

static void
on_stream_deck_active_page_changed_cb (BsStreamDeck       *stream_deck,
                                       GParamSpec         *pspec,
                                       BsStreamDeckEditor *self)
{
  GtkFlowBoxChild *flowbox_child;
  BsPage *active_page;
  BsPage *parent;

  BS_ENTRY;

  active_page = bs_stream_deck_get_active_page (stream_deck);
  parent = bs_page_get_parent (active_page);
  flowbox_child = gtk_flow_box_get_child_at_index (self->buttons_flowbox, parent ? 1 : 0);
  g_assert (flowbox_child != NULL);

  gtk_flow_box_select_child (self->buttons_flowbox, flowbox_child);

  BS_EXIT;
}

static void
on_stream_deck_button_icon_changed_cb (BsStreamDeckButton *stream_deck_button,
                                       BsIcon             *icon,
                                       BsStreamDeckEditor *self)
{
  GtkFlowBoxChild *child;
  uint8_t position;

  position = bs_stream_deck_button_get_position (stream_deck_button);
  child = gtk_flow_box_get_child_at_index (self->buttons_flowbox, position);

  update_button_texture (self, child);
}


/*
 * GObject overrides
 */

static void
bs_stream_deck_editor_finalize (GObject *object)
{
  BsStreamDeckEditor *self = (BsStreamDeckEditor *)object;

  g_clear_object (&self->stream_deck);

  G_OBJECT_CLASS (bs_stream_deck_editor_parent_class)->finalize (object);
}

static void
bs_stream_deck_editor_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BsStreamDeckEditor *self = BS_STREAM_DECK_EDITOR (object);

  switch (prop_id)
    {
    case PROP_STREAM_DECK:
      g_value_set_object (value, self->stream_deck);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_editor_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BsStreamDeckEditor *self = BS_STREAM_DECK_EDITOR (object);

  switch (prop_id)
    {
    case PROP_STREAM_DECK:
      g_assert (self->stream_deck == NULL);
      self->stream_deck = g_value_dup_object (value);
      build_button_grid (self);
      g_signal_connect_object (self->stream_deck,
                               "notify::active-page",
                               G_CALLBACK (on_stream_deck_active_page_changed_cb),
                               self,
                               0);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_stream_deck_editor_class_init (BsStreamDeckEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_ensure (BS_TYPE_STREAM_DECK_BUTTON_EDITOR);

  object_class->finalize = bs_stream_deck_editor_finalize;
  object_class->get_property = bs_stream_deck_editor_get_property;
  object_class->set_property = bs_stream_deck_editor_set_property;

  properties[PROP_STREAM_DECK] = g_param_spec_object ("stream-deck", NULL, NULL,
                                                      BS_TYPE_STREAM_DECK,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/bs-stream-deck-editor.ui");

  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, button_editor);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, buttons_flowbox);

  gtk_widget_class_bind_template_callback (widget_class, on_flowbox_child_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_flowbox_selected_children_changed_cb);

  gtk_widget_class_set_css_name (widget_class, "streamdeckeditor");
}

static void
bs_stream_deck_editor_init (BsStreamDeckEditor *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
bs_stream_deck_editor_new (BsStreamDeck *stream_deck)
{
  return g_object_new (BS_TYPE_STREAM_DECK_EDITOR,
                       "stream-deck", stream_deck,
                       NULL);
}

BsStreamDeck *
bs_stream_deck_editor_get_stream_deck (BsStreamDeckEditor *self)
{
  g_return_val_if_fail (BS_IS_STREAM_DECK_EDITOR (self), NULL);

  return self->stream_deck;
}
