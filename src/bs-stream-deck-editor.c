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
#include "bs-button-grid-region.h"
#include "bs-debug.h"
#include "bs-dial.h"
#include "bs-dial-grid-region.h"
#include "bs-dial-widget.h"
#include "bs-icon.h"
#include "bs-icon-renderer.h"
#include "bs-page.h"
#include "bs-stream-deck.h"
#include "bs-button.h"
#include "bs-button-editor.h"
#include "bs-button-widget.h"
#include "bs-stream-deck-editor.h"
#include "bs-stream-deck-private.h"
#include "bs-touchscreen-private.h"
#include "bs-touchscreen-region.h"

#include <glib/gi18n.h>
#include <libpeas.h>

struct _BsStreamDeckEditor
{
  AdwBin parent_instance;

  BsButtonEditor *button_editor;
  GtkGrid *regions_grid;

  BsStreamDeck *stream_deck;

  GHashTable *region_to_widget;
};

static void on_flowbox_child_activated_cb (GtkFlowBox         *flowbox,
                                           GtkFlowBoxChild    *child,
                                           BsStreamDeckEditor *self);

static void on_flowbox_selected_children_changed_cb (GtkFlowBox         *flowbox,
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
add_button_grid (BsStreamDeckEditor *self,
                 BsButtonGridRegion *button_grid)
{
  BsDeviceRegion *region;
  GtkFlowBox *buttons_flowbox;
  GListModel *buttons;
  unsigned int grid_columns;

  buttons = bs_button_grid_region_get_buttons (button_grid);
  grid_columns = bs_button_grid_region_get_grid_columns (button_grid);

  buttons_flowbox = g_object_new (GTK_TYPE_FLOW_BOX,
                                  "halign", GTK_ALIGN_CENTER,
                                  "valign", GTK_ALIGN_CENTER,
                                  "homogeneous", TRUE,
                                  "activate-on-single-click", FALSE,
                                  "selection-mode", GTK_SELECTION_SINGLE,
                                  "column-spacing", 18,
                                  "row-spacing", 18,
                                  "min-children-per-line", grid_columns,
                                  "max-children-per-line", grid_columns,
                                  NULL);

  g_signal_connect (buttons_flowbox,
                    "child-activated",
                    G_CALLBACK (on_flowbox_child_activated_cb),
                    self);

  g_signal_connect (buttons_flowbox,
                    "selected-children-changed",
                    G_CALLBACK (on_flowbox_selected_children_changed_cb),
                    self);

  for (unsigned int i = 0; i < g_list_model_get_n_items (buttons); i++)
    {
      g_autoptr (BsButton) button = NULL;
      GtkWidget *widget;

      button = g_list_model_get_item (buttons, i);

      widget = bs_button_widget_new (button);
      gtk_flow_box_append (buttons_flowbox, widget);
    }

  region = BS_DEVICE_REGION (button_grid);
  gtk_grid_attach (self->regions_grid,
                   GTK_WIDGET (buttons_flowbox),
                   bs_device_region_get_column (region),
                   bs_device_region_get_row (region),
                   bs_device_region_get_column_span (region),
                   bs_device_region_get_row_span (region));

  g_hash_table_insert (self->region_to_widget, region, buttons_flowbox);
}

static void
add_dial_grid (BsStreamDeckEditor *self,
               BsDialGridRegion   *dial_grid)
{
  BsDeviceRegion *region;
  GtkFlowBox *dials_flowbox;
  GListModel *dials;
  unsigned int grid_columns;

  dials = bs_dial_grid_region_get_dials (dial_grid);
  grid_columns = bs_dial_grid_region_get_grid_columns (dial_grid);

  dials_flowbox = g_object_new (GTK_TYPE_FLOW_BOX,
                                "valign", GTK_ALIGN_CENTER,
                                "homogeneous", TRUE,
                                "activate-on-single-click", FALSE,
                                "selection-mode", GTK_SELECTION_NONE,
                                "column-spacing", 18,
                                "row-spacing", 18,
                                "min-children-per-line", grid_columns,
                                "max-children-per-line", grid_columns,
                                NULL);

  for (size_t i = 0; i < g_list_model_get_n_items (dials); i++)
    {
      g_autoptr (BsDial) dial = NULL;
      GtkWidget *widget;

      dial = g_list_model_get_item (dials, i);
      widget = bs_dial_widget_new (dial);

      gtk_flow_box_append (dials_flowbox, widget);
    }

  region = BS_DEVICE_REGION (dial_grid);
  gtk_grid_attach (self->regions_grid,
                   GTK_WIDGET (dials_flowbox),
                   bs_device_region_get_column (region),
                   bs_device_region_get_row (region),
                   bs_device_region_get_column_span (region),
                   bs_device_region_get_row_span (region));

  g_hash_table_insert (self->region_to_widget, region, dials_flowbox);
}

static void
add_touchscreen (BsStreamDeckEditor  *self,
                 BsTouchscreenRegion *touchscreen_region)
{
  BsDeviceRegion *region;
  GtkWidget *widget;

  widget = adw_bin_new ();
  gtk_widget_set_size_request (widget, -1, 100);
  gtk_widget_add_css_class (widget, "card");

  region = BS_DEVICE_REGION (touchscreen_region);
  gtk_grid_attach (self->regions_grid,
                   widget,
                   bs_device_region_get_column (region),
                   bs_device_region_get_row (region),
                   bs_device_region_get_column_span (region),
                   bs_device_region_get_row_span (region));

  g_hash_table_insert (self->region_to_widget, region, widget);
}

static void
build_regions (BsStreamDeckEditor *self)
{
  GListModel *regions = bs_stream_deck_get_regions (self->stream_deck);

  for (unsigned int i = 0; i < g_list_model_get_n_items (regions); i++)
    {
      g_autoptr (BsDeviceRegion) region = g_list_model_get_item (regions, i);

      if (BS_IS_BUTTON_GRID_REGION (region))
        add_button_grid (self, BS_BUTTON_GRID_REGION (region));
      else if (BS_IS_DIAL_GRID_REGION (region))
        add_dial_grid (self, BS_DIAL_GRID_REGION (region));
      else if (BS_IS_TOUCHSCREEN_REGION (region))
        add_touchscreen (self, BS_TOUCHSCREEN_REGION (region));
      else
        g_assert_not_reached ();
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
  BsButton *button;
  BsAction *action;

  button = bs_button_widget_get_button (BS_BUTTON_WIDGET (child));
  action = bs_button_get_action (button);

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

  if (child)
    {
      BsButton *button;

      button = bs_button_widget_get_button (BS_BUTTON_WIDGET (child));

      bs_button_editor_set_button (self->button_editor, button);
    }
}

static void
on_stream_deck_active_page_changed_cb (BsStreamDeck       *stream_deck,
                                       GParamSpec         *pspec,
                                       BsStreamDeckEditor *self)
{
  GListModel *regions;
  BsPage *active_page;
  BsPage *parent;

  BS_ENTRY;

  active_page = bs_stream_deck_get_active_page (stream_deck);
  parent = bs_page_get_parent (active_page);

  regions = bs_stream_deck_get_regions (self->stream_deck);
  for (unsigned int i = 0; i < g_list_model_get_n_items (regions); i++)
    {
      g_autoptr (BsDeviceRegion) region = g_list_model_get_item (regions, i);

      if (BS_IS_BUTTON_GRID_REGION (region))
        {
          GtkFlowBoxChild *flowbox_child;
          GtkFlowBox *flowbox;

          flowbox = g_hash_table_lookup (self->region_to_widget, region);
          g_assert (GTK_IS_FLOW_BOX (flowbox));

          flowbox_child = gtk_flow_box_get_child_at_index (flowbox, parent ? 1 : 0);
          g_assert (flowbox_child != NULL);

          gtk_flow_box_select_child (flowbox, flowbox_child);
        }
    }

  BS_EXIT;
}


/*
 * GObject overrides
 */

static void
bs_stream_deck_editor_finalize (GObject *object)
{
  BsStreamDeckEditor *self = (BsStreamDeckEditor *)object;

  g_clear_pointer (&self->region_to_widget, g_hash_table_destroy);
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
      build_regions (self);
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

  g_type_ensure (BS_TYPE_BUTTON_EDITOR);

  object_class->finalize = bs_stream_deck_editor_finalize;
  object_class->get_property = bs_stream_deck_editor_get_property;
  object_class->set_property = bs_stream_deck_editor_set_property;

  properties[PROP_STREAM_DECK] = g_param_spec_object ("stream-deck", NULL, NULL,
                                                      BS_TYPE_STREAM_DECK,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/bs-stream-deck-editor.ui");

  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, button_editor);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, regions_grid);

  gtk_widget_class_set_css_name (widget_class, "streamdeckeditor");
}

static void
bs_stream_deck_editor_init (BsStreamDeckEditor *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->region_to_widget = g_hash_table_new (g_direct_hash, g_direct_equal);
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
