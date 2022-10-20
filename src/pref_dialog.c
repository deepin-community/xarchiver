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

#include <string.h>
#include <unistd.h>
#include "pref_dialog.h"
#include "add_dialog.h"
#include "archive.h"
#include "extract_dialog.h"
#include "interface.h"
#include "main.h"
#include "support.h"

gchar *config_file;
GtkIconTheme *icon_theme;

static gint preferred_format;

static gchar *xa_prefs_choose_program (gboolean flag)
{
	gchar *filename = NULL;
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new (flag ? _("Choose the directory to use") : _("Choose the application to use"),
				      GTK_WINDOW(xa_main_window),
				      flag ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

	gtk_widget_destroy (dialog);

	return filename;
}

static void xa_prefs_combo_changed (GtkComboBox *widget, gpointer data)
{
	gchar *filename, *filename_utf8;
	unsigned short int flag = GPOINTER_TO_UINT(data);

	if (gtk_combo_box_get_active(GTK_COMBO_BOX (widget)) == 1)
	{
		filename = xa_prefs_choose_program(flag);
		if (filename != NULL)
		{
			filename_utf8 = g_filename_display_name(filename);
			gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(GTK_WIDGET(widget)), 0);
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(GTK_WIDGET(widget)), 0, filename_utf8);
			g_free(filename_utf8);
			g_free(filename);
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (widget),0);
	}
}

static void xa_prefs_dialog_set_default_options (Prefs_dialog_data *prefs_data)
{
	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_format),0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_data->prefer_unzip), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->confirm_deletion),TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->store_output),FALSE);

	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_icon_size),0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->show_toolbar),TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->show_location_bar),TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->show_sidebar),TRUE);

	if (!xdg_open)
	{
		gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_web_browser),0);
		gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_editor),0);
		gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_viewer),0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_data->combo_prefered_archiver), 0);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_data->combo_prefered_custom_cmd), 0);
	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_temp_dir),0);
	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_extract_dir),0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->check_save_geometry),FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->allow_sub_dir),FALSE);
	/* Set the default options in the extract dialog */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->ensure_directory), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (extract_window->extract_full),TRUE);
	/* Set the default options in the add dialog */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (add_window->recurse),TRUE);
}

void xa_prefs_iconview_changed (GtkIconView *iconview, Prefs_dialog_data *prefs)
{
	GList *list;
	GtkTreePath *path;
	GtkTreeIter iter;
	guint column = 0;

	list = gtk_icon_view_get_selected_items (iconview);
	if (list == NULL)
		return;

	list = g_list_first (list);
	path = (GtkTreePath*)list->data;

	gtk_tree_model_get_iter (GTK_TREE_MODEL(prefs->prefs_liststore),&iter,path);
	gtk_tree_model_get (GTK_TREE_MODEL(prefs->prefs_liststore),&iter,2,&column,-1);

	gtk_tree_path_free(path);
	g_list_free (list);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(prefs->prefs_notebook), column);
}

Prefs_dialog_data *xa_create_prefs_dialog()
{
	GTK_COMPAT_TOOLTIPS;
	GtkWidget *vbox1, *vbox3, *vbox4, *hbox1, *scrolledwindow1;
	GtkWidget *label1, *label2, *label3, *label4, *label5,*label6, *label7, *label8, *label9, *label10, *table1, *table2;
	GtkTreeIter iter;
	GdkPixbuf *icon_pixbuf;
	GtkTreePath *top;
	Prefs_dialog_data *prefs_data;

	prefs_data = g_new0 (Prefs_dialog_data,1);
	prefs_data->dialog1 = gtk_dialog_new_with_buttons(_("Preferences"),
	                                                  NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
	                                                  NULL);
	icon_theme = gtk_icon_theme_get_default();
	gtk_dialog_set_default_response (GTK_DIALOG (prefs_data->dialog1), GTK_RESPONSE_OK);
	gtk_window_set_position (GTK_WINDOW(prefs_data->dialog1),GTK_WIN_POS_CENTER_ON_PARENT);

	vbox1 = gtk_dialog_get_content_area(GTK_DIALOG(prefs_data->dialog1));
	hbox1 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox1,TRUE,TRUE,10);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), scrolledwindow1, TRUE, TRUE, 6);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy", GTK_POLICY_NEVER,"shadow-type", GTK_SHADOW_IN,"vscrollbar-policy", GTK_POLICY_NEVER, NULL);

	prefs_data->prefs_liststore = gtk_list_store_new (3,GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT);
	gtk_list_store_append (prefs_data->prefs_liststore,&iter);
	icon_pixbuf = gtk_icon_theme_load_icon(icon_theme, "gnome-mime-application-zip", 40, (GtkIconLookupFlags) 0, NULL);
	if (!icon_pixbuf) icon_pixbuf = gtk_icon_theme_load_icon(icon_theme,"package-x-generic",32,GTK_ICON_LOOKUP_FORCE_SIZE,NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("Archive"),2,0,-1);
	if(icon_pixbuf != NULL)
		g_object_unref (icon_pixbuf);

	gtk_list_store_append (prefs_data->prefs_liststore, &iter);
	icon_pixbuf = gtk_widget_render_icon (prefs_data->dialog1, "gtk-leave-fullscreen", GTK_ICON_SIZE_DND, NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("Window"),2,1,-1);
	g_object_unref (icon_pixbuf);

	gtk_list_store_append (prefs_data->prefs_liststore, &iter);
	icon_pixbuf = gtk_widget_render_icon (prefs_data->dialog1, "gtk-execute", GTK_ICON_SIZE_DND, NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("Advanced"),2,2,-1);
	g_object_unref (icon_pixbuf);

	prefs_data->iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL(prefs_data->prefs_liststore));
	g_object_unref (prefs_data->prefs_liststore);

	gtk_icon_view_set_item_orientation(GTK_ICON_VIEW(prefs_data->iconview), GTK_ORIENTATION_VERTICAL);
	gtk_icon_view_set_columns(GTK_ICON_VIEW(prefs_data->iconview), 1);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(prefs_data->iconview), 0);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(prefs_data->iconview), 1);
	gtk_container_add(GTK_CONTAINER(scrolledwindow1), prefs_data->iconview);

	top = gtk_tree_path_new_from_string("0");
	gtk_icon_view_select_path(GTK_ICON_VIEW(prefs_data->iconview), top);
	gtk_tree_path_free(top);

	prefs_data->prefs_notebook = gtk_notebook_new ();
	g_object_set (G_OBJECT (prefs_data->prefs_notebook),"show-border", FALSE,"show-tabs", FALSE,"enable-popup",FALSE,NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), prefs_data->prefs_notebook,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(prefs_data->iconview), "selection-changed", G_CALLBACK(xa_prefs_iconview_changed), prefs_data);

	/* Archive page*/
	vbox4 = gtk_vbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook),vbox4);

	hbox1 = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox4), hbox1, FALSE, TRUE,0);

	label4 = gtk_label_new (_("Preferred archive format"));
	gtk_box_pack_start (GTK_BOX (hbox1), label4, FALSE, FALSE,0);

	prefs_data->combo_prefered_format = gtk_combo_box_text_new();
	gtk_box_pack_start (GTK_BOX (hbox1), prefs_data->combo_prefered_format,FALSE,TRUE,0);

	prefs_data->prefer_unzip = gtk_check_button_new_with_mnemonic(_("Prefer unzip for zip files (requires restart)"));
	gtk_box_pack_start(GTK_BOX(vbox4), prefs_data->prefer_unzip, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_data->prefer_unzip), FALSE);

	prefs_data->confirm_deletion = gtk_check_button_new_with_mnemonic (_("Confirm deletion of files"));
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->confirm_deletion, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->confirm_deletion), FALSE);

	prefs_data->check_sort_filename_column = gtk_check_button_new_with_mnemonic(_("Sort archive by filename"));
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->check_sort_filename_column, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_sort_filename_column), FALSE);
	gtk_widget_set_tooltip_text(prefs_data->check_sort_filename_column, _("The filename column is sorted after loading the archive"));

	prefs_data->store_output = gtk_check_button_new_with_mnemonic (_("Store archiver output"));
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->store_output, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->store_output), FALSE);
	gtk_widget_set_tooltip_text(prefs_data->store_output, _("This option consumes more memory with large archives"));

	label1 = gtk_label_new ("");
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 0), label1);

	/* Window page*/
	table1 = gtk_table_new (5, 2,FALSE);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), table1);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 4);

	label9 = gtk_label_new (_("Icons size (requires restart)"));
	gtk_table_attach (GTK_TABLE (table1), label9, 0, 1, 0, 1,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label9), 0, 0.5);
	prefs_data->combo_icon_size = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_icon_size), _("small"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_icon_size), _("small/medium"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_icon_size), _("medium"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_icon_size), _("large"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_icon_size), _("very large"));
	gtk_table_attach (GTK_TABLE (table1), prefs_data->combo_icon_size, 1, 2, 0, 1,
	                  GTK_FILL, GTK_SHRINK, 0, 0);

	prefs_data->check_show_comment = gtk_check_button_new_with_mnemonic (_("Show archive comment"));
	gtk_widget_set_tooltip_text(prefs_data->check_show_comment, _("If checked the archive comment is shown after the archive is loaded"));
	gtk_table_attach (GTK_TABLE (table1), prefs_data->check_show_comment, 0, 2, 1, 2,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_show_comment), FALSE);

	prefs_data->show_sidebar = gtk_check_button_new_with_mnemonic (_("Show archive tree sidebar"));
	gtk_table_attach (GTK_TABLE (table1), prefs_data->show_sidebar, 0, 2, 2, 3,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->show_sidebar), FALSE);

	prefs_data->show_location_bar = gtk_check_button_new_with_mnemonic (_("Show archive location bar"));
	gtk_table_attach (GTK_TABLE (table1), prefs_data->show_location_bar, 0, 2, 3, 4,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->show_location_bar), FALSE);

	prefs_data->show_toolbar = gtk_check_button_new_with_mnemonic (_("Show toolbar"));
	gtk_table_attach (GTK_TABLE (table1), prefs_data->show_toolbar, 0, 2, 4, 5,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->show_toolbar), FALSE);

	label2 = gtk_label_new ("");
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 1), label2);

	/* Advanced page*/
	vbox3 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox3);
  	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), vbox3);


	table2 = gtk_table_new(9, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (vbox3), table2, TRUE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 1);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 4);

	if (!xdg_open)
	{
		label6 = gtk_label_new (_("Web browser to use:"));
		gtk_table_attach (GTK_TABLE (table2), label6, 0, 1, 0, 1,
		                  GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);
		prefs_data->combo_prefered_web_browser = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_web_browser), "");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_web_browser), _("choose..."));
		g_signal_connect (prefs_data->combo_prefered_web_browser,"changed",G_CALLBACK (xa_prefs_combo_changed),NULL);
		gtk_table_attach (GTK_TABLE (table2), prefs_data->combo_prefered_web_browser, 1, 2, 0, 1,
		                  GTK_FILL, GTK_SHRINK, 0, 0);

		label7 = gtk_label_new (_("Open text files with:"));
		gtk_table_attach (GTK_TABLE (table2), label7, 0, 1, 1, 2,
		                  GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);
		prefs_data->combo_prefered_editor = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_editor), "");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_editor), _("choose..."));
		g_signal_connect (prefs_data->combo_prefered_editor,"changed",G_CALLBACK (xa_prefs_combo_changed),NULL);
		gtk_table_attach (GTK_TABLE (table2), prefs_data->combo_prefered_editor, 1, 2, 1, 2,
		                  GTK_FILL, GTK_SHRINK, 0, 0);

		label8 = gtk_label_new (_("Open image files with:"));
		gtk_table_attach (GTK_TABLE (table2), label8, 0, 1, 2, 3,
		                  GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);
		prefs_data->combo_prefered_viewer = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_viewer), "");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_viewer), _("choose..."));
		g_signal_connect (prefs_data->combo_prefered_viewer,"changed",G_CALLBACK (xa_prefs_combo_changed),NULL);
		gtk_table_attach (GTK_TABLE (table2), prefs_data->combo_prefered_viewer, 1, 2, 2, 3,
		                  GTK_FILL, GTK_SHRINK, 0, 0);

		label8 = gtk_label_new(_("Open archive files with:"));
		gtk_table_attach(GTK_TABLE(table2), label8, 0, 1, 3, 4,
		                 GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(label8), 0, 0.5);
		prefs_data->combo_prefered_archiver = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_archiver), "");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_archiver), _("choose..."));
		g_signal_connect(prefs_data->combo_prefered_archiver, "changed", G_CALLBACK(xa_prefs_combo_changed), NULL);
		gtk_table_attach(GTK_TABLE(table2), prefs_data->combo_prefered_archiver, 1, 2, 3, 4,
		                 GTK_FILL, GTK_SHRINK, 0, 0);
	}

	label8 = gtk_label_new(_("Default custom command:"));
	gtk_table_attach(GTK_TABLE(table2), label8, 0, 1, 4, 5,
	                 GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label8), 0, 0.5);
	prefs_data->combo_prefered_custom_cmd = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_custom_cmd), "");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_custom_cmd), _("choose..."));
	g_signal_connect(prefs_data->combo_prefered_custom_cmd, "changed", G_CALLBACK(xa_prefs_combo_changed), NULL);
	gtk_table_attach(GTK_TABLE(table2), prefs_data->combo_prefered_custom_cmd, 1, 2, 4, 5,
	                 GTK_FILL, GTK_SHRINK, 0, 0);

	label9 = gtk_label_new (_("Preferred temp directory:"));
	gtk_table_attach(GTK_TABLE(table2), label9, 0, 1, 5, 6,
	                 GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label9), 0, 0.5);
	prefs_data->combo_prefered_temp_dir = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_temp_dir), _("/tmp"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_temp_dir), _("choose..."));
	g_signal_connect (prefs_data->combo_prefered_temp_dir,"changed",G_CALLBACK (xa_prefs_combo_changed),GUINT_TO_POINTER(1));
	gtk_table_attach(GTK_TABLE(table2), prefs_data->combo_prefered_temp_dir, 1, 2, 5, 6,
	                 GTK_FILL, GTK_SHRINK, 0, 0);

	label10 = gtk_label_new (_("Preferred extraction directory:"));
	gtk_table_attach(GTK_TABLE(table2), label10, 0, 1, 6, 7,
	                 GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label10), 0, 0.5);
	prefs_data->combo_prefered_extract_dir = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_extract_dir), _("/tmp"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_extract_dir), _("choose..."));
	g_signal_connect (prefs_data->combo_prefered_extract_dir,"changed",G_CALLBACK (xa_prefs_combo_changed),GUINT_TO_POINTER(1));
	gtk_table_attach(GTK_TABLE(table2), prefs_data->combo_prefered_extract_dir, 1, 2, 6, 7,
	                 GTK_FILL, GTK_SHRINK, 0, 0);

	prefs_data->check_save_geometry = gtk_check_button_new_with_mnemonic (_("Save window geometry"));
	gtk_table_attach(GTK_TABLE(table2), prefs_data->check_save_geometry, 0, 2, 7, 8,
	                 GTK_FILL, GTK_FILL,0, 0);

	prefs_data->allow_sub_dir = gtk_check_button_new_with_mnemonic (_("Allow subdirs with drag and drop"));
	gtk_table_attach(GTK_TABLE(table2), prefs_data->allow_sub_dir, 0, 2, 8, 9,
	                 GTK_FILL, (GtkAttachOptions) 0, 0, 0);
	gtk_widget_set_tooltip_text(prefs_data->allow_sub_dir, _("This option includes the subdirectories when you add files with drag and drop"));
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_geometry), FALSE);

	if (!xdg_open)
	{
		label5 = gtk_label_new(_("<span color='red' style='italic'>Please install xdg-utils package so that\nXarchiver can recognize more file types.</span>"));
		gtk_label_set_use_markup (GTK_LABEL (label5), TRUE);
		gtk_box_pack_start (GTK_BOX (vbox3), label5, FALSE, FALSE, 0);
	}
	label3 = gtk_label_new ("");
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 2), label3);
	return prefs_data;
}

void xa_prefs_save_options(Prefs_dialog_data *prefs_data, const char *filename)
{
	gchar *conf;
	gchar *value= NULL;
	gsize len;

	if (access(filename, F_OK) == 0 && access(filename, W_OK) == -1)
		return;

	GKeyFile *xa_key_file = g_key_file_new();

	g_key_file_set_integer (xa_key_file,PACKAGE,"preferred_format",gtk_combo_box_get_active (GTK_COMBO_BOX(prefs_data->combo_prefered_format)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "prefer_unzip", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_data->prefer_unzip)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"confirm_deletion",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_data->confirm_deletion)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"sort_filename_content",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_data->check_sort_filename_column)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"store_output",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_data->store_output)));

	g_key_file_set_integer(xa_key_file, PACKAGE, "icon_size", gtk_combo_box_get_active(GTK_COMBO_BOX(prefs_data->combo_icon_size)) + 2);
	g_key_file_set_boolean (xa_key_file,PACKAGE,"show_archive_comment",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_data->check_show_comment)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"show_sidebar",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_data->show_sidebar)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"show_location_bar",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_data->show_location_bar)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"show_toolbar",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_data->show_toolbar)));

	if (!xdg_open)
	{
		value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_web_browser));
		if (value != NULL)
		{
			g_key_file_set_string (xa_key_file,PACKAGE,"preferred_web_browser",value);
			g_free (value);
		}
		value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_editor));
		if (value != NULL)
		{
			g_key_file_set_string (xa_key_file,PACKAGE,"preferred_editor",value);
			g_free(value);
		}
		value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_viewer));
		if (value != NULL)
		{
			g_key_file_set_string (xa_key_file,PACKAGE,"preferred_viewer",value);
			g_free(value);
		}

		value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_archiver));

		if (value != NULL)
		{
			g_key_file_set_string(xa_key_file, PACKAGE, "preferred_archiver", value);
			g_free(value);
		}
	}

	value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_custom_cmd));

	if (value != NULL)
	{
		g_key_file_set_string(xa_key_file, PACKAGE, "preferred_custom_cmd", value);
		g_free(value);
	}

	value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_temp_dir));
	if (value != NULL)
	{
		g_key_file_set_string (xa_key_file,PACKAGE,"preferred_temp_dir",value);
		g_free(value);
	}
	value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_extract_dir));
	if (value != NULL)
	{
		g_key_file_set_string (xa_key_file,PACKAGE,"preferred_extract_dir",value);
		g_free(value);
	}
	g_key_file_set_integer (xa_key_file,PACKAGE,"allow_sub_dir",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_data->allow_sub_dir)));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_data->check_save_geometry)) )
	{
		/* Main window coords */
		gtk_window_get_position (GTK_WINDOW(xa_main_window),&prefs_data->geometry[0],&prefs_data->geometry[1]);
		gtk_window_get_size (GTK_WINDOW(xa_main_window),&prefs_data->geometry[2],&prefs_data->geometry[3]);
		prefs_data->geometry[4] = gtk_paned_get_position(GTK_PANED(hpaned1));
		g_key_file_set_integer_list(xa_key_file, PACKAGE, "mainwindow", prefs_data->geometry,5);
		/* Extract dialog coords */
		if (prefs_data->size_changed[0]) gtk_window_get_size (GTK_WINDOW(extract_window->dialog1),&prefs_data->extract_dialog[0],&prefs_data->extract_dialog[1]);
		g_key_file_set_integer_list(xa_key_file, PACKAGE, "extract", prefs_data->extract_dialog,2);
		/* Add dialog coords */
		if (prefs_data->size_changed[1]) gtk_window_get_size (GTK_WINDOW(add_window->dialog1),&prefs_data->add_coords[0],&prefs_data->add_coords[1]);
		g_key_file_set_integer_list(xa_key_file, PACKAGE, "add", prefs_data->add_coords,2);
	}
	/* Save the options in the extract dialog */
	g_key_file_set_boolean(xa_key_file, PACKAGE, "ensure_directory", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_window->ensure_directory)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"overwrite",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extract_window->overwrite_check)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"full_path",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extract_window->extract_full)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"touch",    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extract_window->touch)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"fresh",  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extract_window->fresh)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"update",   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extract_window->update)));
	/* Save the options in the add dialog */
	g_key_file_set_boolean (xa_key_file,PACKAGE,"store_path",	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->store_path)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"updadd",		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->update)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"freshen",		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->freshen)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"recurse",   	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->recurse)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"solid_archive",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->solid_archive)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"remove_files", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->remove_files)));

	conf = g_key_file_to_data(xa_key_file, &len, NULL);
	g_file_set_contents(filename, conf, len, NULL);
	g_free (conf);
	g_key_file_free(xa_key_file);
}

void xa_prefs_load_options(Prefs_dialog_data *prefs_data)
{
	gint *coords = NULL;
	gint *extract_coords = NULL;
	gint *add_coords = NULL;
	gchar *value;
	gsize coords_len = 0;
	gchar *config_dir = NULL;
	gchar *xarchiver_config_dir = NULL;
	GKeyFile *xa_key_file = g_key_file_new();
	GError *error = NULL;
	gboolean unzip, toolbar;
	gint idx;

	config_dir = g_strconcat (g_get_home_dir(),"/.config",NULL);
	if (g_file_test(config_dir, G_FILE_TEST_EXISTS) == FALSE)
		g_mkdir_with_parents(config_dir,0700);

	xarchiver_config_dir = g_strconcat(config_dir, "/", PACKAGE, NULL);
	g_free (config_dir);
	if (g_file_test(xarchiver_config_dir, G_FILE_TEST_EXISTS) == FALSE)
		g_mkdir_with_parents(xarchiver_config_dir,0700);

	config_file = g_strconcat(xarchiver_config_dir, "/", PACKAGE, "rc", NULL);
	g_free (xarchiver_config_dir);

	if ( ! g_key_file_load_from_file(xa_key_file,config_file,G_KEY_FILE_KEEP_COMMENTS,NULL) )
	{
		/* Write the config file with the default options */
		xa_prefs_dialog_set_default_options(prefs_data);
		xa_prefs_save_options(prefs_data,config_file);
	}
	else
	{
		preferred_format = g_key_file_get_integer(xa_key_file, PACKAGE, "preferred_format", NULL);
		unzip = g_key_file_get_boolean(xa_key_file, PACKAGE, "prefer_unzip", &error);

		if (error)
		{
			if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
				/* preserve default setting with existing, old config file */
				unzip = TRUE;

			g_error_free(error);
			error = NULL;
		}

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_data->prefer_unzip), unzip);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->confirm_deletion),g_key_file_get_boolean(xa_key_file,PACKAGE,"confirm_deletion",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->store_output),g_key_file_get_boolean(xa_key_file,PACKAGE,"store_output",NULL));

		idx = g_key_file_get_integer(xa_key_file, PACKAGE, "icon_size", NULL) - 2;

		/* preserve setting with existing, old config file */
		if (idx == -2)
			idx = 3;
		if (idx < 0)
			idx = 0;

		gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_data->combo_icon_size), idx);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(prefs_data->check_show_comment),g_key_file_get_boolean(xa_key_file,PACKAGE,"show_archive_comment",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(prefs_data->check_sort_filename_column),g_key_file_get_boolean(xa_key_file,PACKAGE,"sort_filename_content",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(prefs_data->show_sidebar),g_key_file_get_boolean(xa_key_file,PACKAGE,"show_sidebar",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(prefs_data->show_location_bar),g_key_file_get_boolean(xa_key_file,PACKAGE,"show_location_bar",NULL));
		toolbar = g_key_file_get_boolean(xa_key_file,PACKAGE,"show_toolbar",&error);
		if (error)
		{
			if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
				/* preserve default setting with existing, old config file */
				toolbar = TRUE;

			g_error_free(error);
			error = NULL;
		}
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(prefs_data->show_toolbar),toolbar);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->allow_sub_dir),g_key_file_get_boolean(xa_key_file,PACKAGE,"allow_sub_dir",NULL));
		if (!xdg_open)
		{
			value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_web_browser",NULL);
			if (value != NULL)
			{
				gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_web_browser), 0);
				gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_web_browser), value);
				gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_web_browser),0);
				g_free(value);
			}
			value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_editor",NULL);
			if (value != NULL)
			{
				gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_editor), 0);
				gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_editor), 0, value);
				gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_editor),0);
				g_free(value);
			}
			value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_viewer",NULL);
			if (value != NULL)
			{
				gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_viewer), 0);
				gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_viewer), 0, value);
				gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_viewer),0);
				g_free(value);
			}

			value = g_key_file_get_string(xa_key_file, PACKAGE, "preferred_archiver", NULL);

			if (!value)
				value = g_find_program_in_path("xarchiver");

			if (value != NULL)
			{
				gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_archiver), 0);
				gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_archiver), 0, value);
				gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_data->combo_prefered_archiver), 0);
				g_free(value);
			}
		}

		value = g_key_file_get_string(xa_key_file, PACKAGE, "preferred_custom_cmd", NULL);

		if (value != NULL)
		{
			gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_custom_cmd), 0);
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_custom_cmd), 0, value);
			gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_data->combo_prefered_custom_cmd), 0);
			g_free(value);
		}

		value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_temp_dir",NULL);
		if (value != NULL)
		{
			gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_temp_dir), 0);
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_temp_dir), 0, value);
			gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_temp_dir),0);
			g_free(value);
		}
		value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_extract_dir",NULL);
		if (value != NULL)
		{
			gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_extract_dir), 0);
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_extract_dir), 0, value);
			gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_extract_dir),0);
			g_free(value);
		}
		coords = g_key_file_get_integer_list(xa_key_file, PACKAGE, "mainwindow", &coords_len, &error);
		if (error)
		{
			prefs_data->geometry[0] = -1;
			g_error_free(error);
			error = NULL;
		}
		else
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->check_save_geometry),TRUE);
			prefs_data->geometry[0] = coords[0];
			prefs_data->geometry[1] = coords[1];
			prefs_data->geometry[2] = coords[2];
			prefs_data->geometry[3] = coords[3];
			prefs_data->geometry[4] = coords[4];
		}
		extract_coords = g_key_file_get_integer_list(xa_key_file, PACKAGE, "extract", &coords_len, &error);
		if (error)
		{
			prefs_data->extract_dialog[0] = -1;
			g_error_free(error);
			error = NULL;
		}
		else
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->check_save_geometry),TRUE);
			prefs_data->extract_dialog[0] = extract_coords[0];
			prefs_data->extract_dialog[1] = extract_coords[1];
		}
		add_coords = g_key_file_get_integer_list(xa_key_file, PACKAGE, "add", &coords_len, &error);
		if (error)
		{
			prefs_data->add_coords[0] = -1;
			g_error_free(error);
			error = NULL;
		}
		else
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->check_save_geometry),TRUE);
			prefs_data->add_coords[0] = add_coords[0];
			prefs_data->add_coords[1] = add_coords[1];
		}
		/* Load the options in the extract dialog */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->ensure_directory), g_key_file_get_boolean(xa_key_file, PACKAGE, "ensure_directory", NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(extract_window->overwrite_check),g_key_file_get_boolean(xa_key_file,PACKAGE,"overwrite",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(extract_window->extract_full),g_key_file_get_boolean(xa_key_file,PACKAGE,"full_path",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(extract_window->touch),g_key_file_get_boolean(xa_key_file,PACKAGE,"touch",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(extract_window->fresh),g_key_file_get_boolean(xa_key_file,PACKAGE,"fresh",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(extract_window->update),g_key_file_get_boolean(xa_key_file,PACKAGE,"update",NULL));
		/* Load the options in the add dialog */
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->store_path),g_key_file_get_boolean(xa_key_file,PACKAGE,"store_path",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->update),g_key_file_get_boolean(xa_key_file,PACKAGE,"updadd",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->freshen),g_key_file_get_boolean(xa_key_file,PACKAGE,"freshen",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->recurse),g_key_file_get_boolean(xa_key_file,PACKAGE,"recurse",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->solid_archive),g_key_file_get_boolean(xa_key_file,PACKAGE,"solid_archive",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->remove_files),g_key_file_get_boolean(xa_key_file,PACKAGE,"remove_files",NULL));
	}
	g_key_file_free (xa_key_file);
	/* config_file is freed in window.c xa_quit_application */
}

void xa_prefs_adapt_options (Prefs_dialog_data *prefs_data)
{
	xa_combo_box_text_append_compressor_types(GTK_COMBO_BOX_TEXT(prefs_data->combo_prefered_format));
	gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_data->combo_prefered_format), preferred_format);
}

void xa_prefs_apply_options (Prefs_dialog_data *prefs_data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(prefs_data->show_toolbar)))
		gtk_widget_show (toolbar1);
	else
		gtk_widget_hide (toolbar1);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(prefs_data->show_location_bar)))
		gtk_widget_show_all (toolbar2);
	else
		gtk_widget_hide (toolbar2);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(prefs_data->show_sidebar)))
		gtk_widget_show(scrolledwindow2);
	else
		gtk_widget_hide(scrolledwindow2);
}
