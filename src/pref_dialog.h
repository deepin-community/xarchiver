/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#ifndef XARCHIVER_PREF_DIALOG_H
#define XARCHIVER_PREF_DIALOG_H

#include <gtk/gtk.h>

typedef struct Prefs_dialog_data
{
	GtkWidget *dialog1,*dialog_vbox1,*combo_prefered_format, *prefer_unzip;
	GtkWidget *confirm_deletion, *store_output,*combo_archive_view,*combo_icon_size;
	GtkWidget *check_show_comment, *check_sort_filename_column, *show_location_bar, *show_sidebar, *show_toolbar, *combo_prefered_viewer, *combo_prefered_archiver, *combo_prefered_custom_cmd;
	GtkWidget *combo_prefered_web_browser, *combo_prefered_editor, *combo_prefered_temp_dir, *combo_prefered_extract_dir, *allow_sub_dir,*check_save_geometry,*prefs_notebook;
	GtkListStore *prefs_liststore;
	GtkWidget *iconview;
	gint geometry[5];
	gint extract_dialog[2];
	gint add_coords[2];
	gboolean size_changed[2];
} Prefs_dialog_data;

extern gchar *config_file;
extern GtkIconTheme *icon_theme;

Prefs_dialog_data *xa_create_prefs_dialog();
void xa_prefs_adapt_options(Prefs_dialog_data *);
void xa_prefs_apply_options(Prefs_dialog_data *);
void xa_prefs_iconview_changed(GtkIconView *, Prefs_dialog_data *);
void xa_prefs_load_options(Prefs_dialog_data *);
void xa_prefs_save_options(Prefs_dialog_data *, const char *);

#endif
