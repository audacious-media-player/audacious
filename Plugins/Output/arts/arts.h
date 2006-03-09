/*
 *  aRts ouput plugin for xmms
 *
 *  Copyright (C) 2000,2003  Haavard Kvaalen <havardk@xmms.org>
 *
 *  Licenced under GNU GPL version 2.
 *
 *  Audacious port by Giacomo Lozito from develia.org
 *
 */

#ifndef XMMS_ARTS_H
#define XMMS_ARTS_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "audacious/plugin.h"
#include "libaudacious/configfile.h"

struct arts_config
{
	int buffer_size;
};

struct params_info
{
	AFormat format;
	int frequency;
	int channels;

	/* Cache these */
	int bps;
	int resolution;
};

extern struct arts_config artsxmms_cfg;

void artsxmms_init(void);
void artsxmms_about(void);
void artsxmms_configure(void);

void artsxmms_tell_audio( AFormat * , gint * , gint * );

void artsxmms_get_volume(int *l, int *r);
void artsxmms_set_volume(int l, int r);

int artsxmms_playing(void);
int artsxmms_free(void);
void artsxmms_write(void *ptr, int length);
void artsxmms_close(void);
void artsxmms_flush(int time);
void artsxmms_pause(short p);
int artsxmms_open(AFormat fmt, int rate, int nch);
int artsxmms_get_output_time(void);
int artsxmms_get_written_time(void);

int (*arts_get_convert_func(int input))(void **, int);


#endif
