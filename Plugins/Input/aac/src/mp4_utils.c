/*
 * some functions for MP4 files
*/

#include "mp4ff.h"
#include "faad.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "audacious/plugin.h"
#include "libaudacious/titlestring.h"
#include "libaudacious/util.h"

const char *mp4AudioNames[]=
  {
    "MPEG-1 Audio Layers 1,2 or 3",
    "MPEG-2 low biterate (MPEG-1 extension) - MP3",
    "MPEG-2 AAC Main Profile",
    "MPEG-2 AAC Low Complexity profile",
    "MPEG-2 AAC SSR profile",
    "MPEG-4 audio (MPEG-4 AAC)",
    0
  };

/* MPEG-4 Audio types from 14496-3 Table 1.5.1 (from mp4.h)*/
const char *mpeg4AudioNames[]=
  {
    "!!!!MPEG-4 Audio track Invalid !!!!!!!",
    "MPEG-4 AAC Main profile",
    "MPEG-4 AAC Low Complexity profile",
    "MPEG-4 AAC SSR profile",
    "MPEG-4 AAC Long Term Prediction profile",
    "MPEG-4 AAC Scalable",
    "MPEG-4 CELP",
    "MPEG-4 HVXC",
    "MPEG-4 Text To Speech",
    "MPEG-4 Main Synthetic profile",
    "MPEG-4 Wavetable Synthesis profile",
    "MPEG-4 MIDI Profile",
    "MPEG-4 Algorithmic Synthesis and Audio FX profile"
  };

/*
 * find AAC track
 */

int getAACTrack(mp4ff_t *infile)
{
  int i, rc;
  int numTracks = mp4ff_total_tracks(infile);

  printf("total-tracks: %d\n", numTracks);
  for(i=0; i<numTracks; i++){
    unsigned char*	buff = 0;
    unsigned int	buff_size = 0;
    mp4AudioSpecificConfig mp4ASC;

    printf("testing-track: %d\n", i);
    mp4ff_get_decoder_config(infile, i, &buff, &buff_size);
    if(buff){
      rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
      g_free(buff);
      if(rc < 0)
	continue;
      return(i);
    }
  }
  return(-1);
}

char *getMP4title(mp4ff_t *infile, char *filename) {
	char *ret=NULL;
	gchar *value, *path, *temp;

	TitleInput *input;
	XMMS_NEW_TITLEINPUT(input);

	// Fill in the TitleInput with the relevant data
	// from the mp4 file that can be used to display the title.
	mp4ff_meta_get_title(infile, &input->track_name);
        mp4ff_meta_get_artist(infile, &input->performer);
	mp4ff_meta_get_album(infile, &input->album_name);
	if (mp4ff_meta_get_track(infile, &value) && value != NULL) {
		input->track_number = atoi(value);
		g_free(value);
	}
	if (mp4ff_meta_get_date(infile, &value) && value != NULL) {
		input->year = atoi(value);
		g_free(value);
	}
	mp4ff_meta_get_genre(infile, &input->genre);
	mp4ff_meta_get_comment(infile, &input->comment);
	input->file_name = g_strdup(g_basename(filename));
	path = g_strdup(filename);
	temp = strrchr(path, '.');
	if (temp != NULL) {++temp;}
	input->file_ext = g_strdup_printf("%s", temp);
	temp = strrchr(path, '/');
	if (temp) {*temp = '\0';}
	input->file_path = g_strdup_printf("%s/", path);

	// Use the default xmms title format to format the
	// title from the above info.
	ret = xmms_get_titlestring(xmms_get_gentitle_format(), input);

        g_free(input->track_name);
        g_free(input->performer);
        g_free(input->album_name);
        g_free(input->genre);
        g_free(input->comment);
        g_free(input->file_name);
        g_free(input->file_path);
	g_free(input);
	g_free(path);

	return ret;
}
