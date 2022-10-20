/*
 *  Copyright (C) Ingo Brückl
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

#ifndef XARCHIVER_PARSER_H
#define XARCHIVER_PARSER_H

#define GRAB_ITEMS(parts, item, cond)                        \
do                                                           \
{                                                            \
  unsigned int part = 1;                                     \
                                                             \
  do                                                         \
  {                                                          \
    while (*line && (*line == ' ' || *line == '\t')) line++; \
    if (part == 1) item = line;                              \
    while (*line && (cond)) line++;                          \
  }                                                          \
  while (++part <= parts);                                   \
                                                             \
  if (*line) *line++ = 0;                                    \
}                                                            \
while (0)

#define USE_PARSER         \
static void *_archive;     \
static int _pos;           \
unsigned int _len;         \
char *_start = line;       \
                           \
(void) _len;               \
                           \
do                         \
{                          \
  if (_archive != archive) \
  {                        \
    _archive = archive;    \
    _pos = -1;             \
  }                        \
}                          \
while (0)

#define NEXT_ITEMS(parts, item) GRAB_ITEMS(parts, item, *line != ' ' && *line != '\t' && *line != '\n')
#define NEXT_ITEM(item) NEXT_ITEMS(1, item)
#define SKIP_ITEM NEXT_ITEM(line)
#define LAST_ITEM(item)   /* for filenames */ \
do                                            \
{                                             \
  int pos;                                    \
                                              \
  GRAB_ITEMS(1, item, *line != '\n');         \
  pos = item - _start;                        \
                                              \
  if (_pos < 0 || pos < _pos) _pos = pos;     \
  else if (pos > _pos) item = _start + _pos;  \
}                                             \
while (0)

#define IF_ITEM_LINE(string) if ((_len = strlen(string)) && (strncmp(line, string, _len) == 0) && (line += _len))
#define DUPE_ITEM(item) item = g_strdup(g_strstrip(line))

#endif
