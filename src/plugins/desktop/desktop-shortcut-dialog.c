/*
 * desktop-shortcut-dialog.c
 *
 * Copyright 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#include "desktop-shortcut-dialog.h"

#if !GTK_CHECK_VERSION (4, 13, 4)
#define GDK_NO_MODIFIER_MASK 0
#endif

struct _DesktopShortcutDialog
{
  AdwWindow parent_instance;

  GtkWidget *cancel_button;
  GtkWidget *set_button;
  GtkShortcutLabel *shortcut_label;
  GtkStack *stack;

  GtkEventController *key_controller;

  GdkModifierType original_modifiers;
  unsigned int original_keysym;

  GdkModifierType new_modifiers;
  unsigned int new_keysym;

  gboolean system_shortcuts_inhibited;

  GTask *task;
};

G_DEFINE_FINAL_TYPE (DesktopShortcutDialog, desktop_shortcut_dialog, ADW_TYPE_WINDOW)

enum
{
  PROP_0,
  PROP_MODIFIERS,
  PROP_KEYSYM,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];


/*
 * ShortcutResult
 */

typedef struct
{
  GdkModifierType modifiers;
  unsigned int keysym;
} ShortcutResult;

static void
shortcut_result_free (gpointer data)
{
  ShortcutResult *shortcut_result = (ShortcutResult *)data;

  if (!shortcut_result)
    return;

  g_free (shortcut_result);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShortcutResult, shortcut_result_free)


/*
 * Auxiliary functions
 */
static void
normalize_keyval_and_mask (unsigned int     group,
                           unsigned int     keycode,
                           GdkModifierType  mask,
                           unsigned int    *out_keyval,
                           GdkModifierType *out_mask)
{
  guint unmodified_keyval;
  guint shifted_keyval;
  GdkModifierType explicit_modifiers;
  GdkModifierType used_modifiers;

  /* We want shift to always be included as explicit modifier for
   * gnome-shell shortcuts. That's because users usually think of
   * shortcuts as including the shift key rather than being defined
   * for the shifted keyval.
   * This helps with num-row keys which have different keyvals on
   * different layouts for example, but also with keys that have
   * explicit key codes at shift level 0, that gnome-shell would prefer
   * over shifted ones, such the DOLLAR key.
   */
  explicit_modifiers = gtk_accelerator_get_default_mod_mask () | GDK_SHIFT_MASK;
  used_modifiers = mask & explicit_modifiers;

  /* Find the base keyval of the pressed key without the explicit
   * modifiers. */
  gdk_display_translate_key (gdk_display_get_default (),
                             keycode,
                             mask & ~explicit_modifiers,
                             group,
                             &unmodified_keyval,
                             NULL,
                             NULL,
                             NULL);

  /* Normalize num-row keys to the number value. This allows these
   * shortcuts to work when switching between AZERTY and layouts where
   * the numbers are at shift level 0. */
  gdk_display_translate_key (gdk_display_get_default (),
                             keycode,
                             GDK_SHIFT_MASK | (mask & ~explicit_modifiers),
                             group,
                             &shifted_keyval,
                             NULL,
                             NULL,
                             NULL);

  if (shifted_keyval >= GDK_KEY_0 && shifted_keyval <= GDK_KEY_9)
    unmodified_keyval = shifted_keyval;

  /* Normalise <Tab> */
  if (unmodified_keyval == GDK_KEY_ISO_Left_Tab)
    unmodified_keyval = GDK_KEY_Tab;

  if (unmodified_keyval == GDK_KEY_Sys_Req && (used_modifiers & GDK_ALT_MASK) != 0)
    {
      /* HACK: we don't want to use SysRq as a keybinding (but we do
       * want Alt+Print), so we avoid translation from Alt+Print to SysRq */
      unmodified_keyval = GDK_KEY_Print;
    }

  *out_keyval = unmodified_keyval;
  *out_mask = used_modifiers;
}

static void
inhibit_system_shortcuts (DesktopShortcutDialog *self)
{
  GdkSurface *surface;
  GtkNative *native;

  if (self->system_shortcuts_inhibited)
    return;

  native = gtk_widget_get_native (GTK_WIDGET (self));
  surface = gtk_native_get_surface (native);

  if (GDK_IS_TOPLEVEL (surface))
    {
      gdk_toplevel_inhibit_system_shortcuts (GDK_TOPLEVEL (surface), NULL);
      self->system_shortcuts_inhibited = TRUE;
    }
}

static void
uninhibit_system_shortcuts (DesktopShortcutDialog *self)
{
  GdkSurface *surface;
  GtkNative *native;

  if (!self->system_shortcuts_inhibited)
    return;

  native = gtk_widget_get_native (GTK_WIDGET (self));
  surface = gtk_native_get_surface (native);

  if (GDK_IS_TOPLEVEL (surface))
    {
      gdk_toplevel_restore_system_shortcuts (GDK_TOPLEVEL (surface));
      self->system_shortcuts_inhibited = FALSE;
    }
}

static void
review_shortcut (DesktopShortcutDialog *self)
{
  g_autofree char *accelerator = NULL;

  g_assert (self->new_modifiers != GDK_NO_MODIFIER_MASK);
  g_assert (self->new_keysym != 0);

  gtk_stack_set_visible_child_name (self->stack, "review");

  accelerator = gtk_accelerator_name (self->new_keysym, self->new_modifiers);
  gtk_shortcut_label_set_accelerator (self->shortcut_label, accelerator);

  gtk_event_controller_set_propagation_phase (self->key_controller, GTK_PHASE_NONE);
  gtk_widget_set_visible (self->cancel_button, TRUE);
  gtk_widget_set_visible (self->set_button, TRUE);

  uninhibit_system_shortcuts (self);
}


/*
 * Callbacks
 */

static gboolean
on_key_pressed_cb (GtkEventControllerKey *key_controller,
                   unsigned int           keyval,
                   unsigned int           keycode,
                   GdkModifierType        modifiers,
                   DesktopShortcutDialog *self)
{
  GdkModifierType real_modifiers;
  unsigned int real_keysym;
  GdkEvent *event;

  normalize_keyval_and_mask (gtk_event_controller_key_get_group (key_controller),
                             keycode, modifiers,
                             &real_keysym, &real_modifiers);

  event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (key_controller));

  /* A single Escape press cancels the editing */
  if (!gdk_key_event_is_modifier (event) &&
      real_modifiers == GDK_NO_MODIFIER_MASK &&
      real_keysym == GDK_KEY_Escape)
    {
      gtk_window_close (GTK_WINDOW (self));
      return GDK_EVENT_STOP;
    }

  self->new_modifiers = real_modifiers;
  self->new_keysym = real_keysym;

  /* CapsLock isn't supported as a keybinding modifier, so keep it from confusing us */
  self->new_modifiers &= ~GDK_LOCK_MASK;

  if (self->new_modifiers != GDK_NO_MODIFIER_MASK && self->new_keysym != 0)
    review_shortcut (self);

  return GDK_EVENT_STOP;
}

static void
on_set_button_clicked_cb (GtkButton             *button,
                          DesktopShortcutDialog *self)
{
  g_autoptr (ShortcutResult) shortcut_result = NULL;
  g_autoptr (GTask) task = NULL;

  g_assert (self->new_modifiers != GDK_NO_MODIFIER_MASK);
  g_assert (self->new_keysym != 0);

  shortcut_result = g_new0 (ShortcutResult, 1);
  shortcut_result->modifiers = self->new_modifiers;
  shortcut_result->keysym = self->new_keysym;

  task = g_steal_pointer (&self->task);

  gtk_window_close (GTK_WINDOW (self));

  g_task_return_pointer (g_steal_pointer (&task),
                         g_steal_pointer (&shortcut_result),
                         shortcut_result_free);
}


/*
 * GtkWindow overrides
 */

static gboolean
desktop_shortcut_dialog_close_request (GtkWindow *window)
{
  DesktopShortcutDialog *self = DESKTOP_SHORTCUT_DIALOG (window);

  uninhibit_system_shortcuts (self);

  if (self->task)
    {
      g_task_return_new_error (g_steal_pointer (&self->task),
                               G_IO_ERROR,
                               G_IO_ERROR_CANCELLED,
                               "Closed without doing anything");
    }

  return FALSE;
}


/*
 * GtkWidget overrides
 */

static void
desktop_shortcut_dialog_map (GtkWidget *widget)
{
  DesktopShortcutDialog *self = DESKTOP_SHORTCUT_DIALOG (widget);

  GTK_WIDGET_CLASS (desktop_shortcut_dialog_parent_class)->map (widget);

  inhibit_system_shortcuts (self);
}

static void
desktop_shortcut_dialog_unmap (GtkWidget *widget)
{
  DesktopShortcutDialog *self = DESKTOP_SHORTCUT_DIALOG (widget);

  GTK_WIDGET_CLASS (desktop_shortcut_dialog_parent_class)->unmap (widget);

  uninhibit_system_shortcuts (self);
}


/*
 * GObject overrides
 */

static void
desktop_shortcut_dialog_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  DesktopShortcutDialog *self = DESKTOP_SHORTCUT_DIALOG (object);

  switch (prop_id)
    {
    case PROP_MODIFIERS:
      g_value_set_flags (value, self->original_modifiers);
      break;

    case PROP_KEYSYM:
      g_value_set_uint (value, self->original_keysym);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
desktop_shortcut_dialog_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  DesktopShortcutDialog *self = DESKTOP_SHORTCUT_DIALOG (object);

  switch (prop_id)
    {
    case PROP_MODIFIERS:
      self->original_modifiers = g_value_get_flags (value);
      break;

    case PROP_KEYSYM:
      self->original_keysym = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
desktop_shortcut_dialog_class_init (DesktopShortcutDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkWindowClass *window_class = GTK_WINDOW_CLASS (klass);

  object_class->get_property = desktop_shortcut_dialog_get_property;
  object_class->set_property = desktop_shortcut_dialog_set_property;

  widget_class->map = desktop_shortcut_dialog_map;
  widget_class->unmap = desktop_shortcut_dialog_unmap;

  window_class->close_request = desktop_shortcut_dialog_close_request;

  properties[PROP_MODIFIERS] = g_param_spec_flags ("modifiers", "", "",
                                                   GDK_TYPE_MODIFIER_TYPE,
                                                   GDK_NO_MODIFIER_MASK,
                                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_KEYSYM] = g_param_spec_uint ("keysym", "", "",
                                               0, G_MAXUINT, 0,
                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/feaneron/Boatswain/plugins/desktop/desktop-shortcut-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, DesktopShortcutDialog, cancel_button);
  gtk_widget_class_bind_template_child (widget_class, DesktopShortcutDialog, key_controller);
  gtk_widget_class_bind_template_child (widget_class, DesktopShortcutDialog, set_button);
  gtk_widget_class_bind_template_child (widget_class, DesktopShortcutDialog, shortcut_label);
  gtk_widget_class_bind_template_child (widget_class, DesktopShortcutDialog, stack);

  gtk_widget_class_bind_template_callback (widget_class, on_key_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_set_button_clicked_cb);
}

static void
desktop_shortcut_dialog_init (DesktopShortcutDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

DesktopShortcutDialog *
desktop_shortcut_dialog_new (GdkModifierType modifiers,
                             unsigned int    keysym)
{
  return g_object_new (DESKTOP_TYPE_SHORTCUT_DIALOG,
                       "modifiers", modifiers,
                       "keysym", keysym,
                       NULL);
}

void
desktop_shortcut_dialog_run (DesktopShortcutDialog *self,
                             GtkWindow             *parent,
                             GCancellable          *cancellable,
                             GAsyncReadyCallback    callback,
                             gpointer               user_data)
{
  g_autoptr (GTask) task = NULL;

  g_return_if_fail (DESKTOP_IS_SHORTCUT_DIALOG (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, desktop_shortcut_dialog_run);

  self->task = g_steal_pointer (&task);

  gtk_window_set_transient_for (GTK_WINDOW (self), parent);
  gtk_window_present (GTK_WINDOW (self));
}

gboolean
desktop_shortcut_dialog_run_finish (DesktopShortcutDialog  *self,
                                    GAsyncResult           *result,
                                    GdkModifierType        *out_modifiers,
                                    unsigned int           *out_keysym,
                                    GError                **error)
{
  g_autoptr (ShortcutResult) shortcut_result = NULL;

  g_return_val_if_fail (DESKTOP_IS_SHORTCUT_DIALOG (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  shortcut_result = g_task_propagate_pointer (G_TASK (result), error);
  if (!shortcut_result)
    return FALSE;

  if (out_modifiers)
    *out_modifiers = shortcut_result->modifiers;

  if (out_keysym)
    *out_keysym = shortcut_result->keysym;

  return TRUE;
}
