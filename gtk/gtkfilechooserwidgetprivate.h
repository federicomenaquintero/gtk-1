/* gtkfilechooserwidgetprivate.h
 *
 * Copyright (C) 2015 Red Hat
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen
 */

#ifndef __GTK_FILE_CHOOSER_WIDGET_PRIVATE_H__
#define __GTK_FILE_CHOOSER_WIDGET_PRIVATE_H__

#include <glib.h>
#include "gtkfilechooserwidget.h"
#include "gtkfilechooserstate.h"
#include "gtkfilechooserutils.h"
#include "gtkfilesystem.h"
#include "gtkfilesystemmodel.h"
#include "gtkgesturelongpress.h"

G_BEGIN_DECLS

typedef enum {
  LOAD_EMPTY,                   /* There is no model */
  LOAD_PRELOAD,                 /* Model is loading and a timer is running; model isn't inserted into the tree yet */
  LOAD_LOADING,                 /* Timeout expired, model is inserted into the tree, but not fully loaded yet */
  LOAD_FINISHED                 /* Model is fully loaded and inserted into the tree */
} LoadState;

typedef enum {
  RELOAD_EMPTY,                 /* No folder has been set */
  RELOAD_HAS_FOLDER             /* We have a folder, although it may not be completely loaded yet; no need to reload */
} ReloadState;

typedef enum {
  LOCATION_MODE_PATH_BAR,
  LOCATION_MODE_FILENAME_ENTRY
} LocationMode;

struct _GtkFileChooserWidgetPrivate {
  GtkFileChooserState state;
  GtkFileChooserState goal_state;
  guint sync_state_idle_id;

  GtkFileSystem *file_system;

  /* Save mode widgets */
  GtkWidget *save_widgets;
  GtkWidget *save_widgets_table;

  /* The file browsing widgets */
  GtkWidget *browse_widgets_hpaned;
  GtkWidget *browse_header_revealer;
  GtkWidget *browse_header_stack;
  GtkWidget *browse_files_stack;
  GtkWidget *browse_files_swin;
  GtkWidget *browse_files_tree_view;
  GtkWidget *remote_warning_bar;

  GtkWidget *browse_files_popover;
  GtkWidget *add_shortcut_item;
  GtkWidget *hidden_files_item;
  GtkWidget *size_column_item;
  GtkWidget *copy_file_location_item;
  GtkWidget *visit_file_item;
  GtkWidget *open_folder_item;
  GtkWidget *rename_file_item;
  GtkWidget *trash_file_item;
  GtkWidget *delete_file_item;
  GtkWidget *sort_directories_item;
  GtkWidget *show_time_item;

  GtkWidget *browse_new_folder_button;
  GtkSizeGroup *browse_path_bar_size_group;
  GtkWidget *browse_path_bar;
  GtkWidget *new_folder_name_entry;
  GtkWidget *new_folder_create_button;
  GtkWidget *new_folder_error_label;
  GtkWidget *new_folder_popover;
  GtkWidget *rename_file_name_entry;
  GtkWidget *rename_file_rename_button;
  GtkWidget *rename_file_error_label;
  GtkWidget *rename_file_popover;
  GFile *rename_file_source_file;

  GtkGesture *long_press_gesture;

  GtkFileSystemModel *browse_files_model;
  char *browse_files_last_selected_name;

  GtkWidget *places_sidebar;
  GtkWidget *places_view;

  /* OPERATION_MODE_SEARCH */
  GtkWidget *search_entry;
  GtkWidget *search_spinner;
  guint show_progress_timeout;
  GtkSearchEngine *search_engine;
  GtkQuery *search_query;
  GtkFileSystemModel *search_model;
  GtkFileSystemModel *model_for_search;

  /* OPERATION_MODE_RECENT */
  GtkRecentManager *recent_manager;
  GtkFileSystemModel *recent_model;
  guint load_recent_id;

  GtkWidget *extra_and_filters;
  GtkWidget *filter_combo_hbox;
  GtkWidget *filter_combo;
  GtkWidget *preview_box;
  GtkWidget *preview_label;
  GtkWidget *preview_widget;
  GtkWidget *extra_align;
  GtkWidget *extra_widget;

  GtkWidget *location_entry_box;
  GtkWidget *location_entry;
  LocationMode location_mode;

  GtkWidget *external_entry;

  GtkWidget *choice_box;
  GHashTable *choices;

  /* Handles */
  GCancellable *file_list_drag_data_received_cancellable;
  GCancellable *update_current_folder_cancellable;
  GCancellable *should_respond_get_info_cancellable;
  GCancellable *file_exists_get_info_cancellable;

  LoadState load_state;
  ReloadState reload_state;
  guint load_timeout_id;

  GSList *pending_select_files;

  GtkFileFilter *current_filter;
  GSList *filters;

  GtkBookmarksManager *bookmarks_manager;

  GFile *current_folder;
  GFile *preview_file;
  char *preview_display_name;

  GtkTreeViewColumn *list_name_column;
  GtkCellRenderer *list_name_renderer;
  GtkCellRenderer *list_pixbuf_renderer;
  GtkTreeViewColumn *list_time_column;
  GtkCellRenderer *list_date_renderer;
  GtkCellRenderer *list_time_renderer;
  GtkTreeViewColumn *list_size_column;
  GtkCellRenderer *list_size_renderer;
  GtkTreeViewColumn *list_location_column;
  GtkCellRenderer *list_location_renderer;

  guint location_changed_id;

  gulong settings_signal_id;
  int icon_size;

  GSource *focus_entry_idle;

  gulong toplevel_set_focus_id;
  GtkWidget *toplevel_last_focus_widget;

  GtkFileChooserSettings settings;

  /* Flags */

  guint local_only : 1;
  guint preview_widget_active : 1;
  guint use_preview_label : 1;
  guint show_hidden_set : 1;
  guint do_overwrite_confirmation : 1;
  guint list_sort_ascending : 1;
  guint shortcuts_current_folder_active : 1;
  guint create_folders : 1;
  guint auto_selecting_first_row : 1;
};

void
gtk_file_chooser_widget_set_save_entry (GtkFileChooserWidget *chooser,
                                        GtkWidget            *entry);

G_END_DECLS

#endif /* __GTK_FILE_CHOOSER_WIDGET_PRIVATE_H__ */
