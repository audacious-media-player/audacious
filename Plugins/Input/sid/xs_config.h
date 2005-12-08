#ifndef XS_CONFIG_H
#define XS_CONFIG_H

#include "xmms-sid.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Configuration structure
 */
enum XS_EMUENGINE {
	XS_ENG_SIDPLAY1 = 1,
	XS_ENG_SIDPLAY2
};


enum XS_BUILDER {
	XS_BLD_RESID = 1,
	XS_BLD_HARDSID
};

enum XS_RESOLUTION {
	XS_RES_8BIT = 8,
	XS_RES_16BIT = 16
};

enum XS_CHANNELS {
	XS_CHN_MONO = 1,
	XS_CHN_STEREO = 2,
	XS_CHN_AUTOPAN = 3
};


enum XS_CLOCK {
	XS_CLOCK_PAL = 1,
	XS_CLOCK_NTSC
};


enum XS_MPU {
	XS_MPU_BANK_SWITCHING = 1,
	XS_MPU_TRANSPARENT_ROM,
	XS_MPU_PLAYSID_ENVIRONMENT,
	XS_MPU_REAL
};


enum XS_SSC {
	XS_SSC_NONE = 1,
	XS_SSC_POPUP,
	XS_SSC_PATCH,
	XS_SSC_SEEK
};


enum XS_SIDMODEL {
	XS_SIDMODEL_UNKNOWN = 0,
	XS_SIDMODEL_6581,
	XS_SIDMODEL_8580
};


extern struct t_xs_cfg {
	/* General audio settings */
	gint		audioBitsPerSample;
	gint		audioChannels;
	gint		audioFrequency;

	/* General libSIDPlay settings */
	gboolean	mos8580;		/* TRUE = 8580, FALSE = 6581 */
	gboolean	forceModel;
	gboolean	emulateFilters;
	gfloat		filterFs;
	gfloat		filterFm;
	gfloat		filterFt;
	gint		memoryMode;		/* See XS_MPU-constants */
	gint		clockSpeed;		/* PAL (50Hz) or NTSC (60Hz) */
	gboolean	forceSpeed;		/* TRUE = force to given clockspeed */

	gint		playerEngine;		/* Selected player engine */

	gboolean	sid2OptLevel;		/* SIDPlay2 emulation optimization */
	gint		sid2Builder;		/* SIDPlay2 "builder" aka SID-emu */

	gboolean	oversampleEnable;
	gint		oversampleFactor;	/* Factor of oversampling */

	/* Playing settings */
	gboolean	playMaxTimeEnable,
			playMaxTimeUnknown;	/* Use max-time only when song-length is unknown */
	gint		playMaxTime;		/* MAX playtime in seconds */

	gboolean	playMinTimeEnable;
	gint		playMinTime;		/* MIN playtime in seconds */

	gboolean	songlenDBEnable;
	gchar		*songlenDBPath;		/* Path to Songlengths.txt */


	/* Miscellaneous settings */
	gboolean	stilDBEnable;
	gchar		*stilDBPath;		/* Path to STIL.txt */
	gchar		*hvscPath;		/* Path-prefix for HVSC */

	gint		subsongControl;
	gboolean	detectMagic;

	gboolean	titleOverride;		/* TRUE if XMMS titles are overriden */
	gchar		*titleFormat;

	gboolean	subAutoEnable,
			subAutoMinOnly;
	gint		subAutoMinTime;
} xs_cfg;

XS_MUTEX_H(xs_cfg);

/*
 * Configuration-file
 */
enum {
	CTYPE_INT = 1,
	CTYPE_FLOAT,
	CTYPE_STR,
	CTYPE_BOOL
};

enum {
	WTYPE_BGROUP = 1,
	WTYPE_SPIN,
	WTYPE_SCALE,
	WTYPE_BUTTON,
	WTYPE_TEXT
};


typedef struct {
	gint	itemType;	/* Type of item (CTYPE_*) */
	void	*itemData;	/* Pointer to variable */
	gchar	*itemName;	/* Name of configuration item */
} t_xs_cfg_item;


typedef struct {
	gint	widType;
	gint	itemType;
	gchar	*widName;
	void	*itemData;
	gint	itemSet;
} t_xs_wid_item;


/*
 * Functions
 */
void	xs_init_configuration(void);
void	xs_configure(void);
void	xs_read_configuration(void);
gint	xs_write_configuration(void);


#ifdef __cplusplus
}
#endif
#endif /* XS_CONFIG_H */
