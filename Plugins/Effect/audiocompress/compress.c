/* compress.c
** Compressor logic
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "config.h"
#include "compress.h"

static int *peaks = NULL;
static int gainCurrent, gainTarget;

static struct {
	int anticlip;
	int target;
	int gainmax;
	int gainsmooth;
	int buckets;
} prefs;

void CompressCfg(int anticlip, int target, int gainmax,
		 int gainsmooth, int buckets)
{
	static int lastsize = 0;

	prefs.anticlip = anticlip;
	prefs.target = target;
	prefs.gainmax = gainmax;
	prefs.gainsmooth = gainsmooth;
	prefs.buckets = buckets;

	/* Allocate the peak structure */
	peaks = realloc(peaks, sizeof(int)*prefs.buckets);

	if (prefs.buckets > lastsize)
		memset(peaks + lastsize, 0, sizeof(int)*(prefs.buckets
							 - lastsize));
	lastsize = prefs.buckets;
}

void CompressFree(void)
{
	if (peaks)
        {
		free(peaks);
                peaks = NULL;
        }
}

void CompressDo(void *data, unsigned int length)
{
	int16_t *audio = (int16_t *)data, *ap;
	int peak, pos;
	int i;
	int gr, gf, gn;
	static int pn = -1;
#ifdef STATS
	static int clip = 0;
#endif
	static int clipped = 0;

	if (!peaks)
		return;

	if (pn == -1)
	{
		for (i = 0; i < prefs.buckets; i++)
			peaks[i] = 0;
	}
	pn = (pn + 1)%prefs.buckets;

#ifdef DEBUG
	fprintf(stderr, "modifyNative16(0x%08x, %d)\n",(unsigned int)data,
		length);
#endif

	/* Determine peak's value and position */
	peak = 1;
	pos = 0;

#ifdef DEBUG
	fprintf(stderr, "finding peak(b=%d)\n", pn);
#endif

	ap = audio;
	for (i = 0; i < length/2; i++)
	{
		int val = *ap;
		if (val > peak)
		{
			peak = val;
			pos = i;
		} else if (-val > peak)
		{
			peak = -val;
			pos = i;
		}
		ap++;
	}
	peaks[pn] = peak;

	for (i = 0; i < prefs.buckets; i++)
	{
		if (peaks[i] > peak)
		{
			peak = peaks[i];
			pos = 0;
		}
	}

	/* Determine target gain */
	gn = (1 << GAINSHIFT)*prefs.target/peak;

	if (gn <(1 << GAINSHIFT))
		gn = 1 << GAINSHIFT;

	gainTarget = (gainTarget *((1 << prefs.gainsmooth) - 1) + gn)
				      >> prefs.gainsmooth;

	/* Give it an extra insignifigant nudge to counteract possible
	** rounding error
	*/

	if (gn < gainTarget)
		gainTarget--;
	else if (gn > gainTarget)
		gainTarget++;

	if (gainTarget > prefs.gainmax << GAINSHIFT)
		gainTarget = prefs.gainmax << GAINSHIFT;


	/* See if a peak is going to clip */
	gn = (1 << GAINSHIFT)*32768/peak;

	if (gn < gainTarget)
	{
		gainTarget = gn;

		if (prefs.anticlip)
			pos = 0;

	} else
	{
		/* We're ramping up, so draw it out over the whole frame */
		pos = length;
	}

	/* Determine gain rate necessary to make target */
	if (!pos)
		pos = 1;

	gr = ((gainTarget - gainCurrent) << 16)/pos;

	/* Do the shiznit */
	gf = gainCurrent << 16;

#ifdef STATS
	fprintf(stderr, "\rgain = %2.2f%+.2e ",
		gainCurrent*1.0/(1 << GAINSHIFT),
		(gainTarget - gainCurrent)*1.0/(1 << GAINSHIFT));
#endif

	ap = audio;
	for (i = 0; i < length/2; i++)
	{
		int sample;

		/* Interpolate the gain */
		gainCurrent = gf >> 16;
		if (i < pos)
			gf += gr;
		else if (i == pos)
			gf = gainTarget << 16;

		/* Amplify */
		sample = (*ap)*gainCurrent >> GAINSHIFT;
		if (sample < -32768)
		{
#ifdef STATS
			clip++;
#endif
			clipped += -32768 - sample;
			sample = -32768;
		} else if (sample > 32767)
		{
#ifdef STATS
			clip++;
#endif
			clipped += sample - 32767;
			sample = 32767;
		}
		*ap++ = sample;
	}
#ifdef STATS
	fprintf(stderr, "clip %d b%-3d ", clip, pn);
#endif

#ifdef DEBUG
	fprintf(stderr, "\ndone\n");
#endif
}

