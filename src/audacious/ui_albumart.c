/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "ui_fileinfopopup.h"
#include "main.h"
#include "playback.h"

static gboolean
has_front_cover_extension(const gchar *name)
{
	char *ext;

	ext = strrchr(name, '.');
	if (!ext) {
		/* No file extension */
		return FALSE;
	}

	return g_strcasecmp(ext, ".jpg") == 0 ||
	       g_strcasecmp(ext, ".jpeg") == 0 ||
	       g_strcasecmp(ext, ".png") == 0;
}

static gboolean
cover_name_filter(const gchar *name, const gchar *filter, const gboolean ret_on_empty)
{
	gboolean result = FALSE;
	gchar **splitted;
	gchar *current;
	gchar *lname;
	gint i;

	if (!filter || strlen(filter) == 0) {
		return ret_on_empty;
	}

	splitted = g_strsplit(filter, ",", 0);

	lname = g_strdup(name);
	g_strdown(lname);

	for (i = 0; !result && (current = splitted[i]); i++) {
		gchar *stripped = g_strstrip(g_strdup(current));
		g_strdown(stripped);

		result = result || strstr(lname, stripped);

		g_free(stripped);
	}

	g_free(lname);
	g_strfreev(splitted);

	return result;
}

/* Check wether it's an image we want */
static gboolean
is_front_cover_image(const gchar *imgfile)
{
	return cover_name_filter(imgfile, cfg.cover_name_include, TRUE) &&
	       !cover_name_filter(imgfile, cfg.cover_name_exclude, FALSE);
}

static gboolean
is_file_image(const gchar *imgfile, const gchar *file_name)
{
	char *imgfile_ext, *file_name_ext;
	size_t imgfile_len, file_name_len;

	imgfile_ext = strrchr(imgfile, '.');
	if (!imgfile_ext) {
		/* No file extension */
		return FALSE;
	}

	file_name_ext = strrchr(file_name, '.');
	if (!file_name_ext) {
		/* No file extension */
		return FALSE;
	}

	imgfile_len = (imgfile_ext - imgfile);
	file_name_len = (file_name_ext - file_name);

	if (imgfile_len == file_name_len) {
		return (g_ascii_strncasecmp(imgfile, file_name, imgfile_len) == 0);
	} else {
		return FALSE;
	}
}

gchar*
fileinfo_recursive_get_image(const gchar* path,
	const gchar* file_name, gint depth)
{
	GDir *d;

	if (cfg.recurse_for_cover && depth > cfg.recurse_for_cover_depth)
		return NULL;

	d = g_dir_open(path, 0, NULL);

	if (d) {
		const gchar *f;

		if (cfg.use_file_cover && file_name) {
			/* Look for images matching file name */
			while((f = g_dir_read_name(d))) {
				gchar *newpath = g_strconcat(path, "/", f, NULL);

				if (!g_file_test(newpath, G_FILE_TEST_IS_DIR) &&
				    has_front_cover_extension(f) &&
				    is_file_image(f, file_name)) {
					g_dir_close(d);
					return newpath;
				}

				g_free(newpath);
			}
			g_dir_rewind(d);
		}

		/* Search for files using filter */
		while ((f = g_dir_read_name(d))) {
			gchar *newpath = g_strconcat(path, "/", f, NULL);

			if (!g_file_test(newpath, G_FILE_TEST_IS_DIR) &&
			    has_front_cover_extension(f) &&
			    is_front_cover_image(f)) {
				g_dir_close(d);
				return newpath;
			}

			g_free(newpath);
		}
		g_dir_rewind(d);

		/* checks whether recursive or not. */
		if (!cfg.recurse_for_cover) {
			g_dir_close(d);
			return NULL;
		}

		/* Descend into directories recursively. */
		while ((f = g_dir_read_name(d))) {
			gchar *newpath = g_strconcat(path, "/", f, NULL);

			if(g_file_test(newpath, G_FILE_TEST_IS_DIR)) {
				gchar *tmp = fileinfo_recursive_get_image(newpath,
					NULL, depth + 1);
				if(tmp) {
					g_free(newpath);
					g_dir_close(d);
					return tmp;
				}
			}

			g_free(newpath);
		}

		g_dir_close(d);
	}

	return NULL;
}
