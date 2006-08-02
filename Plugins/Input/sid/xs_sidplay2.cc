/*
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   libSIDPlay v2 support

   Programmed and designed by Matti 'ccr' Hamalainen <ccr@tnsp.org>
   (C) Copyright 1999-2005 Tecnic Software productions (TNSP)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
*/
#include "xmms-sid.h"

#ifdef HAVE_SIDPLAY2

#include "xs_sidplay2.h"
#include <stdio.h>
#include "xs_config.h"
#include "xs_support.h"
#include "xs_length.h"
#include "xs_title.h"

#include <sidplay/sidplay2.h>
#ifdef HAVE_RESID_BUILDER
#include <sidplay/builders/resid.h>
#endif
#ifdef HAVE_HARDSID_BUILDER
#include <sidplay/builders/hardsid.h>
#endif


typedef struct
{
	sidplay2 *currEng;
	sidbuilder *currBuilder;
	sid2_config_t currConfig;
	SidTune *currTune;
} t_xs_sidplay2;


/* We need to 'export' all this pseudo-C++ crap */
extern "C"
{


/* Check if we can play the given file
 */
gboolean xs_sidplay2_isourfile(gchar * pcFilename)
{
	SidTune *testTune = new SidTune(pcFilename);

	if (!testTune) return FALSE;

	if (!testTune->getStatus()) {
		delete testTune;
		return FALSE;
	}

	delete testTune;
	return TRUE;
}



/* Initialize SIDPlay2
 */
gboolean xs_sidplay2_init(t_xs_status * myStatus)
{
	gint tmpFreq;
	t_xs_sidplay2 *myEngine;
	assert(myStatus);

	/* Allocate internal structures */
	myEngine = (t_xs_sidplay2 *) g_malloc0(sizeof(t_xs_sidplay2));
	myStatus->sidEngine = myEngine;
	if (!myEngine) return FALSE;

	/* Initialize the engine */
	myEngine->currEng = new sidplay2;
	if (!myEngine->currEng) {
		XSERR("Could not initialize libSIDPlay2 emulation engine\n");
		return FALSE;
	}

	/* Get current configuration */
	myEngine->currConfig = myEngine->currEng->config();

	/* Configure channels and stuff */
	switch (myStatus->audioChannels) {

	case XS_CHN_AUTOPAN:
		myEngine->currConfig.playback = sid2_stereo;
		break;

	case XS_CHN_STEREO:
		myEngine->currConfig.playback = sid2_stereo;
		break;

	case XS_CHN_MONO:
	default:
		myEngine->currConfig.playback = sid2_mono;
		myStatus->audioChannels = XS_CHN_MONO;
		break;
	}


	/* Memory mode settings */
	switch (xs_cfg.memoryMode) {
	case XS_MPU_BANK_SWITCHING:
		myEngine->currConfig.environment = sid2_envBS;
		break;

	case XS_MPU_TRANSPARENT_ROM:
		myEngine->currConfig.environment = sid2_envTP;
		break;

	case XS_MPU_PLAYSID_ENVIRONMENT:
		myEngine->currConfig.environment = sid2_envPS;
		break;

	case XS_MPU_REAL:
	default:
		myEngine->currConfig.environment = sid2_envR;
		xs_cfg.memoryMode = XS_MPU_REAL;
		break;
	}


	/* Audio parameters sanity checking and setup */
	myEngine->currConfig.precision = myStatus->audioBitsPerSample;
	tmpFreq = myStatus->audioFrequency;

	if (myStatus->oversampleEnable)
		tmpFreq = (tmpFreq * myStatus->oversampleFactor);

	myEngine->currConfig.frequency = tmpFreq;

	switch (myStatus->audioBitsPerSample) {
	case XS_RES_8BIT:
		myStatus->audioFormat = FMT_U8;
		myEngine->currConfig.sampleFormat = SID2_LITTLE_UNSIGNED;
		break;

	case XS_RES_16BIT:
	default:
		switch (myStatus->audioFormat) {
		case FMT_U16_LE:
			myEngine->currConfig.sampleFormat = SID2_LITTLE_UNSIGNED;
			break;

		case FMT_U16_BE:
			myEngine->currConfig.sampleFormat = SID2_BIG_UNSIGNED;
			break;

		case FMT_U16_NE:
#ifdef WORDS_BIGENDIAN
			myEngine->currConfig.sampleFormat = SID2_BIG_UNSIGNED;
#else
			myEngine->currConfig.sampleFormat = SID2_LITTLE_UNSIGNED;
#endif
			break;

		case FMT_S16_LE:
			myEngine->currConfig.sampleFormat = SID2_LITTLE_SIGNED;
			break;

		case FMT_S16_BE:
			myEngine->currConfig.sampleFormat = SID2_BIG_SIGNED;
			break;

		default:
			myStatus->audioFormat = FMT_S16_NE;
#ifdef WORDS_BIGENDIAN
			myEngine->currConfig.sampleFormat = SID2_BIG_SIGNED;
#else
			myEngine->currConfig.sampleFormat = SID2_LITTLE_SIGNED;
#endif
			break;

		}
		break;
	}

	/* Initialize builder object */
	XSDEBUG("init builder #%i, maxsids=%i\n", xs_cfg.sid2Builder, (myEngine->currEng->info()).maxsids);
#ifdef HAVE_RESID_BUILDER
	if (xs_cfg.sid2Builder == XS_BLD_RESID) {
		ReSIDBuilder *rs = new ReSIDBuilder("ReSID builder");
		myEngine->currBuilder = (sidbuilder *) rs;
		if (rs) {
			/* Builder object created, initialize it */
			rs->create((myEngine->currEng->info()).maxsids);
			if (!*rs) {
				XSERR("rs->create() failed. SIDPlay2 suxx again.\n");
				return FALSE;
			}

			rs->filter(xs_cfg.emulateFilters);
			if (!*rs) {
				XSERR("rs->filter(%d) failed.\n", xs_cfg.emulateFilters);
				return FALSE;
			}

			rs->sampling(tmpFreq);
			if (!*rs) {
				XSERR("rs->sampling(%d) failed.\n", tmpFreq);
				return FALSE;
			}

			rs->filter((sid_filter_t *) NULL);
			if (!*rs) {
				XSERR("rs->filter(NULL) failed.\n");
				return FALSE;
			}
		}
	}
#endif
#ifdef HAVE_HARDSID_BUILDER
	if (xs_cfg.sid2Builder == XS_BLD_HARDSID) {
		HardSIDBuilder *hs = new HardSIDBuilder("HardSID builder");
		myEngine->currBuilder = (sidbuilder *) hs;
		if (hs) {
			/* Builder object created, initialize it */
			hs->create((myEngine->currEng->info()).maxsids);
			if (!*hs) {
				XSERR("hs->create() failed. SIDPlay2 suxx again.\n");
				return FALSE;
			}

			hs->filter(xs_cfg.emulateFilters);
			if (!*hs) {
				XSERR("hs->filter(%d) failed.\n", xs_cfg.emulateFilters);
				return FALSE;
			}
		}
	}
#endif

	if (!myEngine->currBuilder) {
		XSERR("Could not initialize SIDBuilder object.\n");
		return FALSE;
	}

	XSDEBUG("%s\n", myEngine->currBuilder->credits());


	/* Clockspeed settings */
	switch (xs_cfg.clockSpeed) {
	case XS_CLOCK_NTSC:
		myEngine->currConfig.clockDefault = SID2_CLOCK_NTSC;
		break;

	case XS_CLOCK_PAL:
	default:
		myEngine->currConfig.clockDefault = SID2_CLOCK_PAL;
		xs_cfg.clockSpeed = XS_CLOCK_PAL;
		break;
	}


	/* Configure rest of the emulation */
	myEngine->currConfig.sidEmulation = myEngine->currBuilder;
	
	if (xs_cfg.forceSpeed) { 
		myEngine->currConfig.clockForced = true;
		myEngine->currConfig.clockSpeed = myEngine->currConfig.clockDefault;
	} else {
		myEngine->currConfig.clockForced = false;
		myEngine->currConfig.clockSpeed = SID2_CLOCK_CORRECT;
	}
	
	if (xs_cfg.sid2OptLevel)
		myEngine->currConfig.optimisation = 1;
	else
		myEngine->currConfig.optimisation = 0;

	if (xs_cfg.mos8580)
		myEngine->currConfig.sidDefault = SID2_MOS8580;
	else
		myEngine->currConfig.sidDefault = SID2_MOS6581;

	if (xs_cfg.forceModel)
		myEngine->currConfig.sidModel = myEngine->currConfig.sidDefault;
	else
		myEngine->currConfig.sidModel = SID2_MODEL_CORRECT;

	myEngine->currConfig.sidSamples = TRUE;	// FIXME FIX ME, make configurable!


	/* Now set the emulator configuration */
	if (myEngine->currEng->config(myEngine->currConfig) < 0) {
		XSERR("Emulator engine configuration failed!\n");
		return FALSE;
	}

	/* Create the sidtune */
	myEngine->currTune = new SidTune(0);
	if (!myEngine->currTune) {
		XSERR("Could not initialize SIDTune object.\n");
		return FALSE;
	}

	return TRUE;
}


/* Close SIDPlay2 engine
 */
void xs_sidplay2_close(t_xs_status * myStatus)
{
	t_xs_sidplay2 *myEngine;
	assert(myStatus);

	myEngine = (t_xs_sidplay2 *) myStatus->sidEngine;

	/* Free internals */
	if (myEngine->currBuilder) {
		delete myEngine->currBuilder;
		myEngine->currBuilder = NULL;
	}

	if (myEngine->currEng) {
		delete myEngine->currEng;
		myEngine->currEng = NULL;
	}

	if (myEngine->currTune) {
		delete myEngine->currTune;
		myEngine->currTune = NULL;
	}

	xs_sidplay2_deletesid(myStatus);

	g_free(myEngine);
	myStatus->sidEngine = NULL;
}


/* Initialize current song and sub-tune
 */
gboolean xs_sidplay2_initsong(t_xs_status * myStatus)
{
	t_xs_sidplay2 *myEngine;
	assert(myStatus);

	myEngine = (t_xs_sidplay2 *) myStatus->sidEngine;
	if (!myEngine) return FALSE;

	if (!myEngine->currTune->selectSong(myStatus->currSong)) {
		XSERR("currTune->selectSong() failed\n");
		return FALSE;
	}

	if (myEngine->currEng->load(myEngine->currTune) < 0) {
		XSERR("currEng->load() failed\n");
		return FALSE;
	}

	return TRUE;
}


/* Emulate and render audio data to given buffer
 */
guint xs_sidplay2_fillbuffer(t_xs_status * myStatus, gchar * audioBuffer, guint audioBufSize)
{
	t_xs_sidplay2 *myEngine;
	assert(myStatus);

	myEngine = (t_xs_sidplay2 *) myStatus->sidEngine;
	if (!myEngine) return 0;

	return myEngine->currEng->play(audioBuffer, audioBufSize);
}


/* Load a given SID-tune file
 */
gboolean xs_sidplay2_loadsid(t_xs_status * myStatus, gchar * pcFilename)
{
	t_xs_sidplay2 *myEngine;
	assert(myStatus);

	myEngine = (t_xs_sidplay2 *) myStatus->sidEngine;
	if (!myEngine) return FALSE;

	/* Try to get the tune */
	if (!pcFilename) return FALSE;

	if (!myEngine->currTune->load(pcFilename))
		return FALSE;

	return TRUE;
}


/* Delete INTERNAL information
 */
void xs_sidplay2_deletesid(t_xs_status * myStatus)
{
	assert(myStatus);

	/* With the current scheme of handling sidtune-loading, we don't do anything here. */
}


/* Return song information
 */
#define TFUNCTION	xs_sidplay2_getsidinfo
#define TTUNEINFO	SidTuneInfo
#define TTUNE		SidTune
#include "xs_sidplay.h"

}				/* extern "C" */
#endif				/* HAVE_SIDPLAY2 */
