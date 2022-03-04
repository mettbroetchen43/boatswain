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

#include "bs-icon.h"
#include "bs-icon-renderer.h"
#include "bs-stream-deck.h"
#include "bs-stream-deck-button.h"
#include "bs-stream-deck-button-editor.h"
#include "bs-stream-deck-editor.h"

#include <glib/gi18n.h>

struct _BsStreamDeckEditor
{
  AdwBin parent_instance;

  GtkAdjustment *brightness_adjustment;
  BsStreamDeckButtonEditor *button_editor;
  GtkFlowBox *buttons_flowbox;
  GtkLabel *firmware_version_label;
  GtkLabel *serial_number_label;

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

static void
update_button_texture (BsStreamDeckEditor *self,
                       GtkFlowBoxChild    *child)
{
  BsStreamDeckButton *stream_deck_button;
  GtkWidget *image;
  BsIcon *icon;
  int position;

  position = gtk_flow_box_child_get_index (child);
  stream_deck_button = bs_stream_deck_get_button (self->stream_deck, position);
  icon = bs_stream_deck_button_get_icon (stream_deck_button);

  image = gtk_flow_box_child_get_child (child);
  gtk_image_set_from_paintable (GTK_IMAGE (image), GDK_PAINTABLE (icon));
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
      GtkWidget *button;
      GtkWidget *image;

      button = gtk_flow_box_child_new ();
      gtk_flow_box_append (self->buttons_flowbox, button);

      image = gtk_image_new ();
      gtk_widget_add_css_class (image, "card");
      gtk_widget_set_size_request (image, 96, 96);
      gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
      gtk_widget_set_overflow (image, GTK_OVERFLOW_HIDDEN);
      gtk_flow_box_child_set_child (GTK_FLOW_BOX_CHILD (button), image);

      stream_deck_button = bs_stream_deck_get_button (self->stream_deck, i);
      g_signal_connect_object (stream_deck_button,
                               "icon-changed",
                               G_CALLBACK (on_stream_deck_button_icon_changed_cb),
                               self,
                               0);

      update_button_texture (self, GTK_FLOW_BOX_CHILD (button));
    }
}

static void
setup_editor (BsStreamDeckEditor *self)
{
  build_button_grid (self);

  gtk_adjustment_set_value (self->brightness_adjustment,
                            bs_stream_deck_get_brightness (self->stream_deck));
  gtk_label_set_label (self->serial_number_label,
                       bs_stream_deck_get_serial_number (self->stream_deck));
  gtk_label_set_label (self->firmware_version_label,
                       bs_stream_deck_get_firmware_version (self->stream_deck));
}


/*
 * Callbacks
 */

static void
on_brightness_adjustment_value_changed_cb (GtkAdjustment      *adjustment,
                                           GParamSpec         *pspec,
                                           BsStreamDeckEditor *self)
{
  bs_stream_deck_set_brightness (self->stream_deck,
                                 gtk_adjustment_get_value (adjustment));
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
      setup_editor (self);
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

  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, brightness_adjustment);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, button_editor);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, buttons_flowbox);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, firmware_version_label);
  gtk_widget_class_bind_template_child (widget_class, BsStreamDeckEditor, serial_number_label);

  gtk_widget_class_bind_template_callback (widget_class, on_brightness_adjustment_value_changed_cb);
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
