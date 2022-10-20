/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Lukasz 'Sil2100' Zemczak - <sil2100@vexillium.org>
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

#ifndef LHA_H
#define LHA_H

#include <glib.h>
#include "archive.h"

void xa_lha_add(XArchive *, GSList *, gchar *);
void xa_lha_ask(XArchive *);
gboolean xa_lha_check_program(gchar *);
void xa_lha_delete(XArchive *, GSList *);
gboolean xa_lha_extract(XArchive *, GSList *);
void xa_lha_list(XArchive *);
void xa_lha_test(XArchive *);

#endif
