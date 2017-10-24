/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */

#include <gtk/gtk.h>
#include "gtkfilechooserwidgetprivate.h"

static GtkWidget *
create_file_chooser_dialog (GtkFileChooserAction action)
{
  GtkWidget *window;
  GtkWidget *dialog;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  dialog = gtk_file_chooser_dialog_new ("Hello",
					GTK_WINDOW (window),
					action,
					"Cancel", GTK_RESPONSE_CANCEL,
					"Accept", GTK_RESPONSE_ACCEPT,
					NULL);

  g_signal_connect_swapped (dialog, "destroy",
			    G_CALLBACK (g_object_unref), window);

  return dialog;
}

static void
test_widget_is_focused_at_startup (GtkFileChooserAction action, const char *instance_name)
{
  GtkWidget *dialog;
  GtkWidget *focused;

  dialog = create_file_chooser_dialog (action);
  gtk_widget_show_now (dialog);

  focused = gtk_window_get_focus (GTK_WINDOW (dialog));
  g_assert (focused != NULL);

  g_assert_cmpstr (g_type_name_from_instance ((GTypeInstance *) focused), ==, instance_name);
}

static void
test_open_file_list_is_focused_at_startup (void)
{
  test_widget_is_focused_at_startup (GTK_FILE_CHOOSER_ACTION_OPEN, "GtkFileChooserListView");
}

static void
test_save_location_entry_is_focused_at_startup (void)
{
  test_widget_is_focused_at_startup (GTK_FILE_CHOOSER_ACTION_SAVE, "GtkFileChooserEntry");
}

/* Copied from _gtk_file_chooser_delegate_get_quark() */
static GQuark
get_file_chooser_delegate_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gtk-file-chooser-delegate");

  return quark;
}

static GtkWidget *
get_file_chooser_widget (GtkFileChooserDialog *dialog)
{
  gpointer obj;

  obj = g_object_get_qdata (G_OBJECT (dialog), get_file_chooser_delegate_quark ());
  g_assert (GTK_IS_FILE_CHOOSER_WIDGET (obj));

  return GTK_WIDGET (obj);
}

static GtkFileChooserWidgetPrivate *
get_file_chooser_widget_private (GtkFileChooserDialog *dialog)
{
  GtkWidget *widget;
  GtkFileChooserWidgetPrivate *priv;

  widget = get_file_chooser_widget (dialog);
  priv = G_TYPE_INSTANCE_GET_PRIVATE (widget, GTK_TYPE_FILE_CHOOSER_WIDGET, GtkFileChooserWidgetPrivate);

  return priv;
}

static void
spin_main_loop ()
{
  int i;

  /* For some reason, the file chooser's sync idle callback doesn't get run if
   * we only run one iteration.  Running it twice works.
   */
  for (i = 2; i; i--)
    {
      gtk_main_iteration ();
    }
}

static void
check_create_folder_button_is_visible (GtkFileChooserDialog *dialog, gboolean should_be_visible)
{
  GtkFileChooserWidgetPrivate *priv;

  priv = get_file_chooser_widget_private (dialog);
  g_assert (priv->browse_new_folder_button != NULL);
  g_assert (gtk_widget_is_visible (priv->browse_new_folder_button) == should_be_visible);

  gtk_file_chooser_set_create_folders (GTK_FILE_CHOOSER (dialog), FALSE);
  spin_main_loop ();

  g_assert (gtk_widget_is_visible (priv->browse_new_folder_button) == FALSE);

  gtk_file_chooser_set_create_folders (GTK_FILE_CHOOSER (dialog), TRUE);
  spin_main_loop ();

  g_assert (gtk_widget_is_visible (priv->browse_new_folder_button) == should_be_visible);
}

static void
test_create_folder_button_is_invisible_in_open (void)
{
  GtkWidget *dialog;

  dialog = create_file_chooser_dialog (GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_show_now (dialog);

  check_create_folder_button_is_visible (GTK_FILE_CHOOSER_DIALOG (dialog), FALSE);
}

static void
test_create_folder_button_is_visible_in_save (void)
{
  GtkWidget *dialog;

  dialog = create_file_chooser_dialog (GTK_FILE_CHOOSER_ACTION_SAVE);
  gtk_widget_show_now (dialog);

  check_create_folder_button_is_visible (GTK_FILE_CHOOSER_DIALOG (dialog), TRUE);
}

int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv);

  g_test_add_func ("/filechooser/open/file_list_is_focused_at_startup",
		   test_open_file_list_is_focused_at_startup);
  g_test_add_func ("/filechooser/save/location_entry_is_focused_at_startup",
		   test_save_location_entry_is_focused_at_startup);

  g_test_add_func ("/filechooser/create_folder_button/is_invisible_in_open",
		   test_create_folder_button_is_invisible_in_open);
  g_test_add_func ("/filechooser/create_folder_button/is_visible_in_save",
		   test_create_folder_button_is_visible_in_save);

  return g_test_run ();
}
