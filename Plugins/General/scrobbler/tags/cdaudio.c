/*
 *   libmetatag - A media file tag-reader library
 *   Copyright (C) 2003, 2004  Pipian
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <musicbrainz/mb_c.h>
#include "include/cdaudio.h"
#include "include/endian.h"
#include "../fmt.h"
#include "../config.h"
#include "include/unicode.h"
#define BUFFER_SIZE 4096

#ifdef MAKE_BMP
#include <libaudacious/vfs.h>
#define vfs_fopen vfs_fopen
#define vfs_fclose vfs_fclose
#define vfs_fread vfs_fread
#define vfs_fseek vfs_fseek
#define ftell vfs_ftell
#define VFSFile VFSFile
#endif

/*
 * Determining if a CD is being played is left up to the reader.
 *
 
int cdaudio_find(char *filename)
{
	We've got to find a way to ensure this works on all platforms. 
	
	return !(fmt_strcasecmp(strrchr(filename, '.') + 1, "cda"));
}
*/

cdaudio_t *readCDAudio(char *filename, char track)
{
	int retVal;
	musicbrainz_t mb;
	char *tmp;
	cdaudio_t *musicbrainz = calloc(sizeof(cdaudio_t), 1);
	
	memset(musicbrainz, 0, sizeof(cdaudio_t));
	
	tmp = malloc(BUFFER_SIZE / 4 + 1);
	mb = mb_New();
	mb_SetDevice(mb, filename);
	pdebug("Submitting query to MusicBrainz...", META_DEBUG);
	retVal = mb_Query(mb, MBQ_GetCDInfo);
	if(retVal == 0)
	{
#ifdef META_DEBUG
		char error[129] = "";
		pdebug("ERROR: Query failed.", META_DEBUG);
		mb_GetQueryError(mb, error, 128);
		pdebug(fmt_vastr("REASON: %s", error), META_DEBUG);
#endif
		mb_Delete(mb);
		free(tmp);
		free(musicbrainz);
		return NULL;
	}
	pdebug("Selecting result...", META_DEBUG);
	retVal = mb_Select1(mb, MBS_SelectAlbum, 1);
	if(retVal == 0)
	{
		pdebug("ERROR: Album select failed.", META_DEBUG);
		mb_Delete(mb);
		free(tmp);
		free(musicbrainz);
		return NULL;
	}
	pdebug("Extracting MusicBrainz data from result...", META_DEBUG);
	memset(tmp, '\0', BUFFER_SIZE / 4 + 1);
	retVal = mb_GetResultData(mb, MBE_AlbumGetAlbumName, tmp, BUFFER_SIZE / 4);
	if(retVal == 0)
	{
		pdebug("ERROR: Album title not found.", META_DEBUG);
		musicbrainz->album = calloc(1, 1);
	}
	else
	{
		musicbrainz->album = malloc(strlen(tmp) + 1);
		strcpy(musicbrainz->album, tmp);
	}
	memset(tmp, '\0', BUFFER_SIZE / 4 + 1);
	retVal = mb_GetResultData1(mb, MBE_AlbumGetArtistName, tmp, BUFFER_SIZE / 4, track);
	if(retVal == 0)
	{
		pdebug("ERROR: Artist name not found.", META_DEBUG);
		musicbrainz->artist = calloc(1, 1);
	}
	else
	{
		musicbrainz->artist = malloc(strlen(tmp) + 1);
		strcpy(musicbrainz->artist, tmp);
	}
	memset(tmp, '\0', BUFFER_SIZE / 4 + 1);
	retVal = mb_GetResultData1(mb, MBE_AlbumGetTrackName, tmp, BUFFER_SIZE / 4, track);
	if(retVal == 0)
	{
		pdebug("ERROR: Track title not found.", META_DEBUG);
		musicbrainz->title = calloc(1, 1);
	}
	else
	{
		musicbrainz->title = malloc(strlen(tmp) + 1);
		strcpy(musicbrainz->title, tmp);
	}
	memset(tmp, '\0', BUFFER_SIZE / 4 + 1);
	retVal = mb_GetResultData1(mb, MBE_AlbumGetTrackId, tmp, BUFFER_SIZE / 4, track);
	if(retVal == 0)
	{
		pdebug("ERROR: MBID not found.", META_DEBUG);
		musicbrainz->mbid = calloc(1, 1);
	}
	else
	{
		musicbrainz->mbid = malloc(64);
		mb_GetIDFromURL(mb, tmp, musicbrainz->mbid, 64);
	}
	mb_Delete(mb);
	free(tmp);
	
	return musicbrainz;
}

void freeCDAudio(cdaudio_t *musicbrainz)
{
	free(musicbrainz->mbid);
	free(musicbrainz->title);
	free(musicbrainz->artist);
	free(musicbrainz->album);
	free(musicbrainz);
}
