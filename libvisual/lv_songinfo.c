/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lv_common.h"
#include "lv_songinfo.h"

static int songinfo_dtor (VisObject *object)
{
	VisSongInfo *songinfo = VISUAL_SONGINFO (object);

	visual_songinfo_free_strings (songinfo);

	if (songinfo->cover != NULL)
		visual_object_unref (VISUAL_OBJECT (songinfo->cover));

	songinfo->cover = NULL;

	return VISUAL_OK;
}

/**
 * @defgroup VisSongInfo VisSongInfo
 * @{
 */

/**
 * Creates a new VisSongInfo structure.
 *
 * @param type Type of interface being used.
 *
 * @return 0 on succes -1 on failure.
 */
VisSongInfo *visual_songinfo_new (VisSongInfoType type)
{
	VisSongInfo *songinfo;

	songinfo = visual_mem_new0 (VisSongInfo, 1);
	
	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (songinfo), TRUE, songinfo_dtor);

	songinfo->type = type;

	return songinfo;
}

/**
 * Frees all the strings within the structure. This frees all the
 * strings used by the structure.
 *
 * @param songinfo Pointer to a VisSongInfo of which the strings need to
 * 	freed.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_free_strings (VisSongInfo *songinfo)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	if (songinfo->songname != NULL)
		visual_mem_free (songinfo->songname);

	if (songinfo->artist != NULL)
		visual_mem_free (songinfo->artist);

	if (songinfo->album != NULL)
		visual_mem_free (songinfo->album);

	if (songinfo->song != NULL)
		visual_mem_free (songinfo->song);

	songinfo->songname = NULL;
	songinfo->artist = NULL;
	songinfo->album = NULL;
	songinfo->song = NULL;

	return VISUAL_OK;
}

/**
 * Sets the interface type to a VisSongInfo. Used to set the interface
 * type to the VisSongInfo structure. The interface type defines if we're
 * providing a simple string containing the song name or an separated
 * set of data containing all the information about a song.
 *
 * @param songinfo Pointer to a VisSongInfo to which the interface type is set.
 * @param type Interface type that is set against the VisSongInfo.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_set_type (VisSongInfo *songinfo, VisSongInfoType type)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);
	
	songinfo->type = type;

	return VISUAL_OK;
}

/**
 * Sets the length of a song. Used to set the length of a song when the
 * advanced interface is being used.
 *
 * @param songinfo Pointer to a VisSongInfo to which the song length is set.
 * @param length The length in seconds.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_set_length (VisSongInfo *songinfo, int length)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);
	
	songinfo->length = length;

	return VISUAL_OK;
}

/**
 * Sets the elapsed time of a song. Used to set the elapsed time of a song when
 * the advanced interface is being used.
 *
 * @param songinfo Pointer to a VisSongInfo to which the elapsed time is set.
 * @param elapsed The elapsed time in seconds.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_set_elapsed (VisSongInfo *songinfo, int elapsed)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	songinfo->elapsed = elapsed;

	return VISUAL_OK;
}

/**
 * Sets a simple song name. Used when the simple interface is being used
 * to set a song name.
 *
 * @param songinfo Pointer to a VisSongInfo to which the simple song name is set.
 * @param name The simple song name.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_set_simple_name (VisSongInfo *songinfo, char *name)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);
	
	if (songinfo->songname != NULL)
		visual_mem_free (songinfo->songname);

	songinfo->songname = strdup (name);

	return VISUAL_OK;
}

/**
 * Sets the artist name. Used to set the artist name when the advanced
 * interface is being used.
 *
 * @param songinfo Pointer to a VisSongInfo to which the artist name is set.
 * @param artist The artist name.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_set_artist (VisSongInfo *songinfo, char *artist)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	if (songinfo->artist != NULL)
		visual_mem_free (songinfo->artist);

	songinfo->artist = strdup (artist);

	return VISUAL_OK;
}

/**
 * Sets the album name. Used to set the album name when the advanced
 * interface is being used.
 *
 * @param songinfo Pointer to a VisSongInfo to which the album name is set.
 * @param album The album name.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_set_album (VisSongInfo *songinfo, char *album)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	if (songinfo->album != NULL)
		visual_mem_free (songinfo->album);

	songinfo->album = strdup (album);

	return VISUAL_OK;
}

/**
 * Sets the song name. Used to set the song name when the advanced
 * interface is being used.
 *
 * @param songinfo Pointer to a VisSongInfo to which the song name is set.
 * @param song The song name.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_set_song (VisSongInfo *songinfo, char *song)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	if (songinfo->song != NULL)
		visual_mem_free (songinfo->song);

	songinfo->song = strdup (song);

	return VISUAL_OK;
}

/**
 * Sets the cover art. Used to set the cover art when the advanced
 * interface is being used.
 *
 * @param songinfo Pointer to a VisSongInfo to which the cover art is set.
 * @param cover Pointer to a VisVideo containing the cover art.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_set_cover (VisSongInfo *songinfo, VisVideo *cover)
{
	VisVideo dtransform;

	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	if (songinfo->cover != NULL)
		visual_object_unref (VISUAL_OBJECT (songinfo->cover));

	/* The coverart image */
	songinfo->cover = visual_video_new ();
	visual_video_set_depth (songinfo->cover, VISUAL_VIDEO_DEPTH_32BIT);
	visual_video_set_dimension (songinfo->cover, 64, 64);
	visual_video_allocate_buffer (songinfo->cover);
	
	/* The temp depth transform video */
	memset (&dtransform, 0, sizeof (VisVideo));

	visual_video_set_depth (&dtransform, VISUAL_VIDEO_DEPTH_32BIT);
	visual_video_set_dimension (&dtransform, cover->width, cover->height);
	visual_video_allocate_buffer (&dtransform);

	visual_video_depth_transform (&dtransform, cover);

	/* Now scale it */
	/* FIXME make cover image size settable ??? */
	visual_video_scale (songinfo->cover, &dtransform, VISUAL_VIDEO_SCALE_BILINEAR);

	/* Unref the depth transform video */
	visual_object_unref (VISUAL_OBJECT (&dtransform));

	return VISUAL_OK;
}

/**
 * Resets the age timer. Used to timestamp a song to the current
 * time.
 *
 * @param songinfo Pointer to a VisSongInfo that is timestamped.
 *
 * @return 0 on succes -1 on failure.
 */
int visual_songinfo_mark (VisSongInfo *songinfo)
{
	visual_log_return_val_if_fail (songinfo != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	visual_timer_start (&songinfo->timer);

	return VISUAL_OK;
}

/**
 * Gives the age of the VisSongInfo. Returns the age in seconds
 * stored in a long.
 *
 * @param songinfo Pointer to a VisSongInfo of which the age is requested.
 *
 * @return The age in seconds.
 */

long visual_songinfo_age (VisSongInfo *songinfo)
{
	VisTime cur;

	visual_time_get (&cur);

	/* Clock has been changed into the past */
	if (cur.tv_sec < songinfo->timer.start.tv_sec)
		visual_songinfo_mark (songinfo);

	visual_time_difference (&cur, &songinfo->timer.start, &cur);

	return cur.tv_sec;
}

/**
 * Copies the content of a VisSongInfo. Used to copy the content of
 * a VisSongInfo in that of another.
 *
 * @param dest Pointer to the destination VisSongInfo.
 * @param src Pointer to the source VisSongInfo.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_SONGINFO_NULL on failure.
 */
int visual_songinfo_copy (VisSongInfo *dest, VisSongInfo *src)
{
	visual_log_return_val_if_fail (dest != NULL, -VISUAL_ERROR_SONGINFO_NULL);
	visual_log_return_val_if_fail (src != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	dest->type = src->type;
	dest->length = src->length;
	dest->elapsed = src->elapsed;

	visual_mem_copy (&dest->timer, &src->timer, sizeof (VisTimer));

	if (src->songname != NULL)
		dest->songname = strdup (src->songname);
	
	if (src->artist != NULL)
		dest->artist = strdup (src->artist);
	
	if (src->album != NULL)
		dest->album = strdup (src->album);
	
	if (src->song != NULL)
		dest->song = strdup (src->song);

	/* Point the coverart VisVideo to that of the original and up it's refcount */
	dest->cover = src->cover;

	if (src->cover != NULL)
		visual_object_ref (VISUAL_OBJECT (src->cover));

	return VISUAL_OK;
}

/**
 * Compares the VisSongInfo strings. Used to compare the content
 * of two VisSongInfos by comparing their strings. This can be used
 * to detect if there is a song change.
 *
 * @param s1 Pointer to the first VisSongInfo.
 * @param s2 Pointer to the second VisSongInfo.
 *
 * @return FALSE on different, TRUE on same, -VISUAL_ERROR_SONGINFO_NULL on failure.
 */
int visual_songinfo_compare (VisSongInfo *s1, VisSongInfo *s2)
{
	int diff = 0;

	visual_log_return_val_if_fail (s1 != NULL, -VISUAL_ERROR_SONGINFO_NULL);
	visual_log_return_val_if_fail (s2 != NULL, -VISUAL_ERROR_SONGINFO_NULL);

	if (s1->songname != NULL && s2->songname != NULL) {
		if (strcmp (s1->songname, s2->songname) != 0)
			diff++;
	} else if ((s1->songname == NULL || s2->songname == NULL) &&
			!(s1->songname == NULL && s2->songname == NULL)) {
		diff++;
	}

	if (s1->artist != NULL && s2->artist != NULL) {
		if (strcmp (s1->artist, s2->artist) != 0)
			diff++;
	} else if ((s1->artist == NULL || s2->artist == NULL) &&
			!(s1->artist == NULL && s2->artist == NULL)) {
		diff++;
	}

	if (s1->album != NULL && s2->album != NULL) {
		if (strcmp (s1->album, s2->album) != 0)
			diff++;
	} else if ((s1->album == NULL || s2->album == NULL) &&
			!(s1->album == NULL && s2->album == NULL)) {
		diff++;
	}

	if (s1->song != NULL && s2->song != NULL) {
		if (strcmp (s1->song, s2->song) != 0)
			diff++;
	} else if ((s1->song == NULL || s2->song == NULL) &&
			!(s1->song == NULL && s2->song == NULL)) {
		diff++;
	}

	return (diff > 0 ? FALSE : TRUE);
}

/**
 * @}
 */

