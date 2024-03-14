/*
 * bs-button-grid-widget.c
 *
 * Copyright 2024 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#define G_LOG_DOMAIN "Button Grid Widget"

#include "bs-button-grid-widget.h"

#include "bs-action-private.h"
#include "bs-button-private.h"
#include "bs-button-grid-region.h"
#include "bs-button-widget.h"
#include "bs-debug.h"

#include <libpeas.h>

struct _BsButtonGridWidget
{
  GtkWidget parent_instance;

  GtkFlowBox *flowbox;

  BsButtonGridRegion *button_grid;
};

G_DEFINE_FINAL_TYPE (BsButtonGridWidget, bs_button_grid_widget, GTK_TYPE_WIDGET)

enum
{
  BUTTON_SELECTED,
  N_SIGNALS,
};

enum
{
  PROP_0,
  PROP_BUTTON_GRID,
  N_PROPS,
};

static guint signals [N_SIGNALS];
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


/*
 * Callbacks
 */

static void
on_flowbox_child_activated_cb (GtkFlowBox         *flowbox,
                               GtkFlowBoxChild    *child,
                               BsButtonGridWidget *self)
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
                                         BsButtonGridWidget *self)
{
  g_autoptr (GList) selected_children = NULL;
  BsButtonWidget *child;

  selected_children = gtk_flow_box_get_selected_children (flowbox);
  child = selected_children ? selected_children->data : NULL;
  g_assert (BS_IS_BUTTON_WIDGET (child));

  if (child)
    g_signal_emit (self, signals[BUTTON_SELECTED], 0, bs_button_widget_get_button (child));
}


/*
 * GObject overrides
 */

static void
bs_button_grid_widget_finalize (GObject *object)
{
  BsButtonGridWidget *self = (BsButtonGridWidget *)object;

  g_clear_object (&self->button_grid);

  G_OBJECT_CLASS (bs_button_grid_widget_parent_class)->finalize (object);
}

static void
bs_button_grid_widget_constructed (GObject *object)
{
  BsButtonGridWidget *self = (BsButtonGridWidget *)object;
  GListModel *buttons;
  unsigned int grid_columns;

  G_OBJECT_CLASS (bs_button_grid_widget_parent_class)->constructed (object);

  grid_columns = bs_button_grid_region_get_grid_columns (self->button_grid);
  gtk_flow_box_set_min_children_per_line (self->flowbox, grid_columns);
  gtk_flow_box_set_max_children_per_line (self->flowbox, grid_columns);

  buttons = bs_button_grid_region_get_buttons (self->button_grid);

  for (unsigned int i = 0; i < g_list_model_get_n_items (buttons); i++)
    {
      g_autoptr (BsButton) button = NULL;
      GtkWidget *widget;

      button = g_list_model_get_item (buttons, i);

      widget = bs_button_widget_new (button);
      gtk_flow_box_append (self->flowbox, widget);
    }
}

static void
bs_button_grid_widget_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BsButtonGridWidget *self = BS_BUTTON_GRID_WIDGET (object);

  switch (prop_id)
    {
    case PROP_BUTTON_GRID:
      g_value_set_object (value, self->button_grid);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_button_grid_widget_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BsButtonGridWidget *self = BS_BUTTON_GRID_WIDGET (object);

  switch (prop_id)
    {
    case PROP_BUTTON_GRID:
      g_assert (self->button_grid == NULL);
      self->button_grid = g_value_dup_object (value);
      g_assert (self->button_grid != NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bs_button_grid_widget_class_init (BsButtonGridWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bs_button_grid_widget_finalize;
  object_class->constructed = bs_button_grid_widget_constructed;
  object_class->get_property = bs_button_grid_widget_get_property;
  object_class->set_property = bs_button_grid_widget_set_property;

  properties[PROP_BUTTON_GRID] = g_param_spec_object ("button-grid", NULL, NULL,
                                                      BS_TYPE_BUTTON_GRID_REGION,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals[BUTTON_SELECTED] = g_signal_new ("button-selected",
                                           BS_TYPE_BUTTON_GRID_WIDGET,
                                           G_SIGNAL_RUN_LAST,
                                           0, NULL, NULL, NULL,
                                           G_TYPE_NONE,
                                           1,
                                           BS_TYPE_BUTTON);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/bs-button-grid-widget.ui");

  gtk_widget_class_bind_template_child (widget_class, BsButtonGridWidget, flowbox);

  gtk_widget_class_bind_template_callback (widget_class, on_flowbox_child_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_flowbox_selected_children_changed_cb);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_css_name (widget_class, "buttongridwidget");
}

static void
bs_button_grid_widget_init (BsButtonGridWidget *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
bs_button_grid_widget_new (BsButtonGridRegion *button_grid)
{
  g_assert (BS_IS_BUTTON_GRID_REGION (button_grid));

  return g_object_new (BS_TYPE_BUTTON_GRID_WIDGET,
                       "button-grid", button_grid,
                       NULL);
}
