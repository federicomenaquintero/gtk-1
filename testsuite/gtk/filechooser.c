/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */

#include <gtk/gtk.h>

static GtkWidget *
create_save_dialog (void)
{
  GtkWidget *window;
  GtkWidget *dialog;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  dialog = gtk_file_chooser_dialog_new ("Hello",
					GTK_WINDOW (window),
					GTK_FILE_CHOOSER_ACTION_SAVE,
					"Cancel", GTK_RESPONSE_CANCEL,
					"Save", GTK_RESPONSE_ACCEPT,
					NULL);

  g_signal_connect_swapped (dialog, "destroy",
			    G_CALLBACK (g_object_unref), window);

  return dialog;
}

static void
test_save_location_entry_is_focused_at_startup (void)
{
  GtkWidget *dialog;
  GtkWidget *focused;

  dialog = create_save_dialog ();
  gtk_widget_show_now (dialog);

  focused = gtk_window_get_focus (GTK_WINDOW (dialog));
  g_assert (focused != NULL);

  g_assert_cmpstr (g_type_name_from_instance ((GTypeInstance *) focused), ==, "GtkFileChooserEntry");
}

int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv);

  g_test_add_func ("/filechooser/save/location_entry_is_focused_at_startup",
		   test_save_location_entry_is_focused_at_startup);

  return g_test_run ();
}
