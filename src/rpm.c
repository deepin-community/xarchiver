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

#include <errno.h>
#include <string.h>
#include <glib/gprintf.h>
#include "rpm.h"
#include "date_utils.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

#define LEAD_LEN 96
#define HDRSIG_MAGIC_LEN 3
#define HDRSIG_VERSION_LEN 1
#define HDRSIG_RESERVED_LEN 4
#define HDRSIG_LEAD_IN_LEN (HDRSIG_MAGIC_LEN + HDRSIG_VERSION_LEN + HDRSIG_RESERVED_LEN)
#define SIGNATURE_START (LEAD_LEN + HDRSIG_LEAD_IN_LEN)
#define HDRSIG_ENTRY_INFO_LEN 8
#define HDRSIG_ENTRY_INDEX_LEN 16

void xa_rpm_ask (XArchive *archive)
{
	archive->can_extract = TRUE;
	archive->can_full_path[0] = TRUE;
	archive->can_touch = TRUE;
	archive->can_overwrite = TRUE;
}

static gchar *xa_rpm2cpio (XArchive *archive)
{
	unsigned char bytes[HDRSIG_ENTRY_INFO_LEN];
	int datalen, entries;
	long offset;
	gchar *cpio_z, *ibs, *command, *executable;
	FILE *stream;
	gboolean success;
	ArchiveType xa;

	signal(SIGPIPE, SIG_IGN);
	stream = fopen(archive->path[0], "r");

	if (stream == NULL)
	{
		gchar *msg, *err;

		msg = g_strdup_printf(_("Can't open RPM file %s:"), archive->path[0]);
		err = g_strconcat(msg, " ", g_strerror(errno), NULL);

		g_free(msg);

		return err;
	}

	/* Signature section */
	if (fseek(stream, SIGNATURE_START, SEEK_CUR) == -1)
	{
		fclose (stream);
		return g_strconcat(_("Can't fseek to position 104:"), " ", g_strerror(errno), NULL);
	}
	if (fread(bytes, sizeof(*bytes), HDRSIG_ENTRY_INFO_LEN, stream) != HDRSIG_ENTRY_INFO_LEN)
	{
		fclose ( stream );
		return g_strconcat(_("Can't read data from file:"), " ", g_strerror(errno), NULL);
	}
	entries = 256 * (256 * (256 * bytes[0] + bytes[1]) + bytes[2]) + bytes[3];
	datalen = 256 * (256 * (256 * bytes[4] + bytes[5]) + bytes[6]) + bytes[7];
	datalen += (16 - (datalen % 16)) % 16;  // header section is aligned
	offset = HDRSIG_ENTRY_INDEX_LEN * entries + datalen;

	/* Header section */
	if (fseek(stream, offset, SEEK_CUR))
	{
		fclose (stream);
		return g_strconcat(_("Can't fseek in file:"), " ", g_strerror(errno), NULL);
	}
	if (fread(bytes, sizeof(*bytes), HDRSIG_ENTRY_INFO_LEN, stream) != HDRSIG_ENTRY_INFO_LEN)
	{
		fclose ( stream );
		return g_strconcat(_("Can't read data from file:"), " ", g_strerror(errno), NULL);
	}
	entries = 256 * (256 * (256 * bytes[0] + bytes[1]) + bytes[2]) + bytes[3];
	datalen = 256 * (256 * (256 * bytes[4] + bytes[5]) + bytes[6]) + bytes[7];
	offset = HDRSIG_ENTRY_INDEX_LEN * entries + datalen;
	offset += ftell(stream);  // offset from top

	fclose(stream);

	/* create a unique temp dir in /tmp */
	if (!xa_create_working_directory(archive))
		return g_strdup("");

	cpio_z = g_strconcat(archive->working_dir, "/xa-tmp.cpio_z", NULL);
	ibs = g_strdup_printf("%lu", offset);

	/* run dd to have the payload (compressed cpio archive) in /tmp */
	command = g_strconcat("dd if=", archive->path[1], " ibs=", ibs, " skip=1 of=", cpio_z, NULL);
	g_free(ibs);

	success = xa_run_command(archive, command);
	g_free(command);

	if (!success)
	{
		g_free(cpio_z);
		return g_strdup("");
	}

	xa = xa_detect_archive_type(cpio_z);

	switch (xa.type)
	{
		case XARCHIVETYPE_GZIP:
			executable = "gzip -dc ";
			break;

		case XARCHIVETYPE_BZIP2:
			executable = "bzip2 -dc ";
			break;

		case XARCHIVETYPE_LZMA:
		case XARCHIVETYPE_XZ:
			executable = "xz -dc ";
			break;

		default:
			g_free(cpio_z);
			return g_strdup(_("Unknown compression type!"));
	}

	command = g_strconcat("sh -c \"exec ", executable, cpio_z, " > ", archive->working_dir, "/xa-tmp.cpio\"", NULL);
	g_free(cpio_z);

	success = xa_run_command(archive, command);
	g_free(command);

	return (success ? NULL : g_strdup(""));
}

static void xa_cpio_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gchar *filename, time[6];
	gpointer item[7];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);

	/* Permissions */
	line[10] = '\0';
	item[4] = line;
	a = 11;

	/* Hard Link */
	for(n=a; n < linesize && line[n] == ' '; ++n);
	line[++n] = '\0';
	//item[5] = line + a;
	n++;
	a = n;

	/* Owner */
	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[5] = line + a;
	n++;

	/* Group */
	for(; n < linesize && line[n] == ' '; ++n);
	a = n;

	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[6] = line + a;
	n++;

	/* Size */
	for(; n < linesize && line[n] == ' '; ++n);
	a = n;

	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[1] = line + a;
	n++;

	/* Date and Time */
	line[54] = '\0';
	item[2] = line + n;
	n = 55;

	/* Time */
	if (((char *) item[2])[9] == ':')
	{
		memcpy(time, item[2] + 7, 5);
		time[5] = 0;
	}
	else
		strcpy(time, "-----");

	item[2] = date_MMM_dD_HourYear(item[2]);
	item[3] = time;

	line[linesize-1] = '\0';
	filename = line + n;

	/* Symbolic link */
	gchar *temp = g_strrstr (filename,"->");
	if (temp)
	{
		a = 3;
		gint len = strlen(filename) - strlen(temp);
		item[0] = filename + a + len;
		filename[strlen(filename) - strlen(temp)] = '\0';
	}
	else
		item[0] = NULL;

	if(line[0] == 'd')
	{
		/* Work around for cpio, which does
		 * not output / with directories */

		if(line[linesize-2] != '/')
			filename = g_strconcat(line + n, "/", NULL);
		else
			filename = g_strdup(line + n);
	}
	else
		filename = g_strdup(line + n);

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		if (!entry->is_dir)
			archive->files++;

		archive->files_size += g_ascii_strtoull(item[1], NULL, 0);
	}

	g_free (filename);
}

void xa_rpm_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Points to"), _("Original Size"), _("Date"), _("Time"), _("Permissions"), _("Owner"), _("Group")};
	gchar *result, *command;
	guint i;

	result = xa_rpm2cpio(archive);

	archive->files_size = 0;
	archive->files = 0;

	archive->columns = 10;
	archive->size_column = 3;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);

	if (result != NULL)
	{
		if (*result)
			command = g_strconcat("sh -c \"echo ", result, " >&2; exit 1\"", NULL);
		else
			command = g_strdup("sh -c \"\"");

		g_free(result);
	}
	else
		command = g_strconcat(archiver[archive->type].program[0], " -tv -I ", archive->working_dir, "/xa-tmp.cpio", NULL);

	archive->parse_output = xa_cpio_parse_output;
	xa_spawn_async_process (archive,command);
	g_free(command);
}

/*
 * Note: cpio lists ' ' as '\ ', '"' as '\"' and '\' as '\\' while it
 * extracts ' ', '"' and '\' respectively, i.e. file names containing
 * one of these three characters can't be handled entirely.
 */

gboolean xa_rpm_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *extract_to, *command;
	gboolean result;

	if (archive->do_full_path)
		extract_to = g_strdup(archive->extraction_dir);
	else
	{
		if (!xa_create_working_directory(archive))
			return FALSE;

		extract_to = g_strconcat(archive->working_dir, "/xa-tmp.XXXXXX", NULL);

		if (!g_mkdtemp(extract_to))
		{
			g_free(extract_to);
			return FALSE;
		}
	}

	files = xa_quote_filenames(file_list, "*?[]\"", FALSE);
	archive->child_dir = g_strdup(extract_to);
	command = g_strconcat(archiver[archive->type].program[0], " -id",
	                      archive->do_touch ? "" : " -m",
	                      archive->do_overwrite ? " -u" : "",
	                      " -I ", archive->working_dir, "/xa-tmp.cpio",
	                      files->str, NULL);
	result = xa_run_command(archive, command);
	g_free(command);

	g_free(archive->child_dir);
	archive->child_dir = NULL;

	/* collect all files that have been extracted to move them without full path */
	if (result && !archive->do_full_path)
	{
		GString *all_files = xa_collect_files_in_dir(extract_to);

		archive->child_dir = g_strdup(extract_to);
		command = g_strconcat("mv",
		                      archive->do_overwrite ? " -f" : " -n",
		                      all_files->str, " ", archive->extraction_dir, NULL);
		g_string_free(all_files, TRUE);

		result = xa_run_command(archive, command);
		g_free(command);

		g_free(archive->child_dir);
		archive->child_dir = NULL;
	}

	g_free(extract_to);
	g_string_free(files,TRUE);

	return result;
}
