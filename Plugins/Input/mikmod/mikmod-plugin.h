#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <gtk/gtk.h>
#include "audacious/plugin.h"
#include <mikmod.h>

extern InputPlugin mikmod_ip;

enum {
	SAMPLE_FREQ_44 = 0,
	SAMPLE_FREQ_22,
	SAMPLE_FREQ_11,
};

typedef struct
{
	gint mixing_freq;
	gint volumefadeout;
	gint surround;
	gint force8bit;
	gint hidden_patterns;
	gint force_mono;
	gint interpolation;
	gint def_pansep;
}
MIKMODConfig;

extern MIKMODConfig mikmod_cfg;

extern MDRIVER drv_xmms;


