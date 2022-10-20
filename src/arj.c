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
#include "arj.h"
#include "date_utils.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static gboolean data_line, fname_line;

void xa_arj_ask (XArchive *archive)
{
	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = archiver[archive->type].is_compressor;
	archive->can_delete = archiver[archive->type].is_compressor;
	archive->can_sfx = archiver[archive->type].is_compressor;
	archive->can_password = archiver[archive->type].is_compressor;
	archive->can_full_path[0] = archiver[archive->type].is_compressor;
	archive->can_full_path[1] = archiver[archive->type].is_compressor;
	archive->can_overwrite = TRUE;
	archive->can_update[0] = TRUE;
	archive->can_update[1] = archiver[archive->type].is_compressor;
	archive->can_freshen[0] = archiver[archive->type].is_compressor;
	archive->can_freshen[1] = archiver[archive->type].is_compressor;
	archive->can_move = archiver[archive->type].is_compressor;
}

static gchar *xa_arj_password_str (XArchive *archive)
{
	if (archive->password && archiver[archive->type].is_compressor)
		return g_strconcat(" -g", archive->password, NULL);
	else
		return g_strdup("");
}

static void xa_arj_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[7];
	unsigned int linesize,n,a;
	static gchar *filename;
	gboolean unarj, lfn, dir, encrypted;

	unarj = !archiver[archive->type].is_compressor;

	if (!data_line)
	{
		if (line[0] == '-')
		{
			data_line = TRUE;
			return;
		}
		return;
	}

	if (!fname_line)
	{
		linesize = strlen(line);
		line[linesize - 1] = '\0';

		if (!unarj && (*line == ' '))
			return;

		if (line[0] == '-' && linesize == (unarj ? 59 : 41))
		{
			data_line = FALSE;
			return;
		}

		if (unarj)
		{
			/* simple column separator check */
			lfn = (linesize < 76 || line[34] != ' ' || line[40] != ' ' || line[49] != ' ' || line[58] != ' ' || line[67] != ' ');

			if (lfn)
				filename = g_strdup(line);
			else
			{
				filename = g_strchomp(g_strndup(line, 12));

				if (!*filename)
				{
					g_free(filename);
					filename = g_strdup(" ");   // just a wild guess in order to have an entry
				}
			}
		}
		else
		{
			lfn = TRUE;
			filename = g_strdup(line + 5);
		}

		fname_line = TRUE;
		if (lfn)
			return;
	}

	if (fname_line)
	{
		linesize = strlen(line);
		/* Size */
		for(n=12; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[0] = line + a;
		n++;

		/* Compressed */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[1] = line + a;
		n++;

		/* Ratio */
    	line[40] = '\0';
    	item[2] = line + 35;

		/* Date */
		line[49] = '\0';
		item[3] = date_YY_MM_DD(line + 41);

		/* Time */
		line[58] = '\0';
		item[4] = line + 50;

		/* CRC */
		if (unarj)
		{
			line[67] = '\0';
			item[6] = line + 59;
		}

		/* Permissions */
		line[unarj ? 72 : 69] = '\0';
		item[5] = line + (unarj ? 68 : 59);

		/* BTPMGVX */
		if (unarj)
		{
			dir = (line[73] == 'D');
			encrypted = (line[76] == 'G');
		}
		/* BPMGS */
		else
		{
			dir = (line[59] == 'd');
			encrypted = (line[77] == '1');
		}

		if (encrypted)
			archive->has_password = TRUE;

		if (unarj && dir)
			/* skip entry since unarj lacks directory structure information */
			entry = NULL;
		else
			entry = xa_set_archive_entries_for_each_row(archive, filename, item);

		if (entry)
		{
			if (dir)
				entry->is_dir = TRUE;

			entry->is_encrypted	= encrypted;

			if (!entry->is_dir)
				archive->files++;

			archive->files_size += g_ascii_strtoull(item[0], NULL, 0);
		}

		g_free(filename);
		fname_line = FALSE;
	}
}

void xa_arj_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, archiver[archive->type].is_compressor ? G_TYPE_POINTER : G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original Size"), _("Compressed"), _("Ratio"), _("Date"), _("Time"), archiver[archive->type].is_compressor ? _("Permissions") : _("Attributes"), archiver[archive->type].is_compressor ? NULL : _("Checksum")};
	guint i;

	data_line = FALSE;
	fname_line = FALSE;
	gchar *command = g_strconcat(archiver[archive->type].program[0], archiver[archive->type].is_compressor ? " v " : " l ", archive->path[1], NULL);
	archive->files_size = 0;
	archive->files = 0;
	archive->parse_output = xa_arj_parse_output;
	xa_spawn_async_process (archive,command);
	g_free (command);

	archive->columns = (archiver[archive->type].is_compressor ? 9 : 10);
	archive->size_column = 2;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_arj_test (XArchive *archive)
{
	gchar *password_str, *command;

	password_str = xa_arj_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " t", password_str, archiver[archive->type].is_compressor ?  " -i -y " : " ", archive->path[1], NULL);
	g_free(password_str);

	xa_run_command(archive, command);
	g_free(command);
}

gboolean xa_arj_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	gboolean result;

	files = xa_quote_filenames(file_list, "*?[]", FALSE);

	if (archiver[archive->type].is_compressor)
	{
		gchar *password_str = xa_arj_password_str(archive);
		command = g_strconcat(archiver[archive->type].program[0],
		                      archive->do_full_path ? " x" : " e",
		                      archive->do_overwrite ? "" : (archive->do_update ? " -u" : (archive->do_freshen ? " -f" : " -n")),
		                      password_str, " -i -y ",
		                      archive->path[1], " ",
		                      archive->extraction_dir, files->str, NULL);
		g_free(password_str);
	}
	else
	{
		if (xa_create_working_directory(archive))
		{
			gchar *files_str, *extraction_dir, *archive_path, *move;

			files_str = xa_escape_bad_chars(files->str, "\"");
			extraction_dir = xa_quote_shell_command(archive->extraction_dir, FALSE);
			archive_path = xa_quote_shell_command(archive->path[0], TRUE);

			if (strcmp(archive->extraction_dir, archive->working_dir) == 0)
				move = g_strdup("");
			else
				move = g_strconcat(" && mv",
				                   archive->do_overwrite ? " -f" : " -n",
				                   archive->do_update ? " -fu" : "",
				                   *files->str ? files_str : " *", " ",
				                   extraction_dir, NULL);

			archive->child_dir = g_strdup(archive->working_dir);
			command = g_strconcat("sh -c \"exec rm -f * && ",
			                      archiver[archive->type].program[0], " e ",
			                      archive_path, move, "\"", NULL);
			g_free(move);
			g_free(archive_path);
			g_free(extraction_dir);
			g_free(files_str);
		}
		else
			command = g_strdup("sh -c \"\"");
	}

	g_string_free(files,TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	g_free(archive->child_dir);
	archive->child_dir = NULL;

	return result;
}

void xa_arj_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *password_str, *command;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	if (!compression)
		compression = "1";

	files = xa_quote_filenames(file_list, "*?[]", FALSE);
	password_str = xa_arj_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_update ? " u" : " a",
	                      archive->do_freshen ? " -f" : "",
	                      archive->do_move ? " -d1" : "",
	                      " -m", compression,
	                      password_str, " -2d -i -y ",
	                      archive->path[1], files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_arj_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, "*?[]", FALSE);
	command = g_strconcat(archiver[archive->type].program[0], " d -i -y ", archive->path[1], files->str, NULL);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
