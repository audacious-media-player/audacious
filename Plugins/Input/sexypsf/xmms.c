/*  sexyPSF - PSF1 player
 *  Copyright (C) 2002-2004 xodnizel
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "audacious/output.h"
#include "audacious/plugin.h"
#include "libaudacious/titlestring.h"
#include "libaudacious/util.h"
#include "libaudacious/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "driver.h"

#define CMD_SEEK	0x80000000
#define CMD_STOP	0x40000000

#define uint32 u32
#define int16 short

static volatile uint32 command;
static volatile int playing=0;
static volatile int nextsong=0;

extern InputPlugin sexypsf_ip;
static char *fnsave=NULL;

static gchar *get_title_psf(gchar *fn);
static int paused;
static GThread *dethread;
static PSFINFO *PSFInfo=NULL;


InputPlugin *get_iplugin_info(void)
      {
         sexypsf_ip.description = "sexyPSF PSF1 Player 0.4.8";
         return &sexypsf_ip;
      }

static int is_our_file(gchar *filename) {
	VFSFile *file;
	gchar magic[4];
	
	// Filter out psflib [we use them, but we can't play them]
	gchar *tmps;
	static const gchar *teststr = "psflib";
	if (strlen(teststr) < strlen(filename)) {
		tmps = filename + strlen(filename);
		tmps -= strlen(teststr);
		if (!strcasecmp(tmps, teststr))
		return 0;
	}
	if ((file = vfs_fopen(filename,"rb"))) {
		vfs_fread(magic,1,4,file);
		//Only allow PSF1 for now.
		if (!strncmp(magic,"PSF\x01",4)) {
			vfs_fclose(file);
			return 1;
		}
		vfs_fclose(file);
	}
	return 0;
}

static void SI(gchar *filename)
{
	gchar *name = get_title_psf(filename);
	sexypsf_ip.set_info(name,PSFInfo->length,44100*2*2*8,44100,2);
	g_free(name);
}

void sexypsf_update(unsigned char *Buffer, long count)
{
	int mask = ~((((16 / 8) * 2)) - 1);

	while(count>0)
	{
		int t=sexypsf_ip.output->buffer_free() & mask;
		if(t>count)		
			produce_audio(sexypsf_ip.output->written_time(), FMT_S16_NE, 2, count, Buffer, NULL);
		else
		{
			if(t)
				produce_audio(sexypsf_ip.output->written_time(), FMT_S16_NE, 2, t, Buffer, NULL);
			usleep((count-t)*1000*5/441/2);
		}
		count-=t;
		Buffer+=t;
	}
	if(command&CMD_SEEK)
	{
		int t=(command&~(CMD_SEEK|CMD_STOP))*1000;

		if(sexypsf_seek(t))
			sexypsf_ip.output->flush(t);
		else	// Negative time!  Must make a C time machine.
		{
			sexypsf_stop();
			return;
		}
		command&=~CMD_SEEK;
	}
	if(command&CMD_STOP)
		sexypsf_stop();
}

static void *sexypsf_playloop(void *arg)
{
dofunky:

	sexypsf_execute();

	/* We have reached the end of the song. Now what... */
	sexypsf_ip.output->buffer_free();
	sexypsf_ip.output->buffer_free();

	while(!(command&CMD_STOP)) 
	{
		if(command&CMD_SEEK)
			{
			int t=(command&~(CMD_SEEK|CMD_STOP))*1000;
			sexypsf_ip.output->flush(t);
			if(!(PSFInfo=sexypsf_load(fnsave)))
				break;
			sexypsf_seek(t); 
			command&=~CMD_SEEK;
			goto dofunky;
			}
		if(!sexypsf_ip.output->buffer_playing()) break;
			usleep(2000);
	}
	sexypsf_ip.output->close_audio();
	if(!(command&CMD_STOP)) nextsong=1;
	g_thread_exit(NULL);
	return(NULL);
}

static void sexypsf_xmms_play(char *fn)
{
	if(playing)
		return;
	nextsong=0;
	paused = 0;
	if(!sexypsf_ip.output->open_audio(FMT_S16_NE, 44100, 2))
	{
		puts("Error opening audio.");
		return;
	}
	fnsave=malloc(strlen(fn)+1);
	strcpy(fnsave,fn);
	if(!(PSFInfo=sexypsf_load(fn)))
	{
		sexypsf_ip.output->close_audio();
		nextsong=1;
	}
 	else
	{
		command=0;
		SI(fn);
		playing=1;
		dethread = g_thread_create((GThreadFunc)sexypsf_playloop,NULL,TRUE,NULL);
	}
}

static void sexypsf_xmms_stop(void)
{
	if(!playing) return;

	if(paused)
		sexypsf_ip.output->pause(0);
	paused = 0;

	command=CMD_STOP;
	g_thread_join(dethread);
	playing = 0;

	if(fnsave)
	{
		free(fnsave);
		fnsave=NULL;
	} 
	sexypsf_freepsfinfo(PSFInfo);
	PSFInfo=NULL;
}

static void sexypsf_xmms_pause(short p)
{
	if(!playing) return;
	sexypsf_ip.output->pause(p);
	paused = p;
}

static void sexypsf_xmms_seek(int time)
{
	if(!playing) return;
	command=CMD_SEEK|time;
}

static int sexypsf_xmms_gettime(void)
{
	if(nextsong)
		return(-1);
	if(!playing) return(0);
		return sexypsf_ip.output->output_time();
}

static void sexypsf_xmms_getsonginfo(char *fn, char **title, int *length)
{
	PSFINFO *tmp;

	if((tmp=sexypsf_getpsfinfo(fn))) {
		*length = tmp->length;
		*title = get_title_psf(fn);
		sexypsf_freepsfinfo(tmp);
	}
}

static gchar *get_title_psf(gchar *fn) {
	gchar *title;
	PSFINFO *tmp = sexypsf_getpsfinfo(fn);
	
	if (tmp->length) {
		TitleInput *tinput;
		
		tinput = bmp_title_input_new();
		
		tinput->performer = g_strdup(tmp->artist);
		tinput->album_name = g_strdup(tmp->game);
		tinput->track_name = g_strdup(tmp->title);
		tinput->file_name = g_path_get_basename(fn);
		tinput->file_path = g_path_get_dirname(fn);
		
		title = xmms_get_titlestring(xmms_get_gentitle_format(),
				tinput);
		g_free(tinput);
	}
	else
		title = g_path_get_basename(fn);
	
	return title;
}

InputPlugin sexypsf_ip =
{
	0,
	0,
	"Plays PSF1 files.",
	0,
	0,
	0,
	is_our_file,
	0,
	sexypsf_xmms_play,
	sexypsf_xmms_stop,
	sexypsf_xmms_pause,
	sexypsf_xmms_seek,
	0,
	sexypsf_xmms_gettime,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	sexypsf_xmms_getsonginfo,
	0,
	0
};
