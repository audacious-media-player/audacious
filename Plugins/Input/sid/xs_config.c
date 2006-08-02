/*  
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   Configuration dialog
   
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#include "xs_config.h"
#include "libaudacious/configdb.h"
#include <stdio.h>
#include "xs_glade.h"
#include "xs_interface.h"
#include "xs_support.h"

/*
 * Global widgets
 */
static GtkWidget *xs_configwin = NULL,
	*xs_sldb_fileselector = NULL,
	*xs_stil_fileselector = NULL,
	*xs_hvsc_pathselector = NULL;

#define LUW(x)	lookup_widget(xs_configwin, x)


/*
 * Configuration specific stuff
 */
GStaticMutex xs_cfg_mutex = G_STATIC_MUTEX_INIT;
struct t_xs_cfg xs_cfg;

t_xs_cfg_item xs_cfgtable[] = {
{ CTYPE_INT,	&xs_cfg.audioBitsPerSample,	"audioBitsPerSample" },
{ CTYPE_INT,	&xs_cfg.audioChannels,		"audioChannels" },
{ CTYPE_INT,	&xs_cfg.audioFrequency,		"audioFrequency" },

{ CTYPE_BOOL,	&xs_cfg.mos8580,		"mos8580" },
{ CTYPE_BOOL,	&xs_cfg.forceModel,		"forceModel" },
{ CTYPE_BOOL,	&xs_cfg.emulateFilters,		"emulateFilters" },
{ CTYPE_FLOAT,	&xs_cfg.filterFs,		"filterFs" },
{ CTYPE_FLOAT,	&xs_cfg.filterFm,		"filterFm" },
{ CTYPE_FLOAT,	&xs_cfg.filterFt,		"filterFt" },
{ CTYPE_INT,	&xs_cfg.memoryMode,		"memoryMode" },
{ CTYPE_INT,	&xs_cfg.clockSpeed,		"clockSpeed" },
{ CTYPE_BOOL,	&xs_cfg.forceSpeed,		"forceSpeed" },

{ CTYPE_INT,	&xs_cfg.playerEngine,		"playerEngine" },

{ CTYPE_INT,	&xs_cfg.sid2Builder,		"sid2Builder" },
{ CTYPE_INT,	&xs_cfg.sid2OptLevel,		"sid2OptLevel" },

{ CTYPE_BOOL,	&xs_cfg.oversampleEnable,	"oversampleEnable" },
{ CTYPE_INT,	&xs_cfg.oversampleFactor,	"oversampleFactor" },

{ CTYPE_BOOL,	&xs_cfg.playMaxTimeEnable,	"playMaxTimeEnable" },
{ CTYPE_BOOL,	&xs_cfg.playMaxTimeUnknown,	"playMaxTimeUnknown" },
{ CTYPE_INT,	&xs_cfg.playMaxTime,		"playMaxTime" },
{ CTYPE_BOOL,	&xs_cfg.playMinTimeEnable,	"playMinTimeEnable" },
{ CTYPE_INT,	&xs_cfg.playMinTime,		"playMinTime" },
{ CTYPE_BOOL,	&xs_cfg.songlenDBEnable,	"songlenDBEnable" },
{ CTYPE_STR,	&xs_cfg.songlenDBPath,		"songlenDBPath" },

{ CTYPE_BOOL,	&xs_cfg.stilDBEnable,		"stilDBEnable" },
{ CTYPE_STR,	&xs_cfg.stilDBPath,		"stilDBPath" },
{ CTYPE_STR,	&xs_cfg.hvscPath,		"hvscPath" },

{ CTYPE_INT,	&xs_cfg.subsongControl,		"subsongControl" },
{ CTYPE_BOOL,	&xs_cfg.detectMagic,		"detectMagic" },

{ CTYPE_BOOL,	&xs_cfg.titleOverride,		"titleOverride" },
{ CTYPE_STR,	&xs_cfg.titleFormat,		"titleFormat" },

{ CTYPE_BOOL,	&xs_cfg.subAutoEnable,		"subAutoEnable" },
{ CTYPE_BOOL,	&xs_cfg.subAutoMinOnly,		"subAutoMinOnly" },
{ CTYPE_INT,	&xs_cfg.subAutoMinTime,		"subAutoMinTime" },
};

static const gint xs_cfgtable_max = (sizeof(xs_cfgtable) / sizeof(t_xs_cfg_item));


t_xs_wid_item xs_widtable[] = {
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_res_16bit",	&xs_cfg.audioBitsPerSample,	XS_RES_16BIT },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_res_8bit",		&xs_cfg.audioBitsPerSample,	XS_RES_8BIT },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_chn_mono",		&xs_cfg.audioChannels,		XS_CHN_MONO },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_chn_stereo",	&xs_cfg.audioChannels,		XS_CHN_STEREO },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_chn_autopan",	&xs_cfg.audioChannels,		XS_CHN_AUTOPAN },
{ WTYPE_SPIN,	CTYPE_INT,	"cfg_samplerate",	&xs_cfg.audioFrequency,		0 },
{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_oversample",	&xs_cfg.oversampleEnable,	0 },
{ WTYPE_SPIN,	CTYPE_INT,	"cfg_oversample_factor",&xs_cfg.oversampleFactor,	0 },

{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_sidplay1",	&xs_cfg.playerEngine,		XS_ENG_SIDPLAY1 },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_sidplay2",	&xs_cfg.playerEngine,		XS_ENG_SIDPLAY2 },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_mem_real",	&xs_cfg.memoryMode,		XS_MPU_REAL },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_mem_banksw",	&xs_cfg.memoryMode,		XS_MPU_BANK_SWITCHING },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_mem_transrom",	&xs_cfg.memoryMode,		XS_MPU_TRANSPARENT_ROM },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_mem_playsid",	&xs_cfg.memoryMode,		XS_MPU_PLAYSID_ENVIRONMENT },

{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_emu_mos8580",	&xs_cfg.mos8580,		0 },
{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_emu_sid_force",	&xs_cfg.forceModel,		0 },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_clock_ntsc",	&xs_cfg.clockSpeed,		XS_CLOCK_NTSC },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_clock_pal",	&xs_cfg.clockSpeed,		XS_CLOCK_PAL },
{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_emu_clock_force",	&xs_cfg.forceSpeed,		0 },
{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_emu_sidplay2_opt",	&xs_cfg.sid2OptLevel,		0 },

{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_sidplay2_resid",&xs_cfg.sid2Builder,		XS_BLD_RESID },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_emu_sidplay2_hardsid",&xs_cfg.sid2Builder,		XS_BLD_HARDSID },

{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_emu_filters",	&xs_cfg.emulateFilters,		0 },
{ WTYPE_SCALE,	CTYPE_FLOAT,	"cfg_emu_filt_fs",	&xs_cfg.filterFs,		0 },
{ WTYPE_SCALE,	CTYPE_FLOAT,	"cfg_emu_filt_fm",	&xs_cfg.filterFm,		0 },
{ WTYPE_SCALE,	CTYPE_FLOAT,	"cfg_emu_filt_ft",	&xs_cfg.filterFt,		0 },

{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_maxtime_enable",	&xs_cfg.playMaxTimeEnable,	0 },
{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_maxtime_unknown",	&xs_cfg.playMaxTimeUnknown,	0 },
{ WTYPE_SPIN,	CTYPE_INT,	"cfg_maxtime",		&xs_cfg.playMaxTime,		0 },
{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_mintime_enable",	&xs_cfg.playMinTimeEnable,	0 },
{ WTYPE_SPIN,	CTYPE_INT,	"cfg_mintime",		&xs_cfg.playMinTime,		0 },
{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_sld_enable",	&xs_cfg.songlenDBEnable,	0 },
{ WTYPE_TEXT,	CTYPE_STR,	"cfg_sld_dbpath",	&xs_cfg.songlenDBPath,		0 },

{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_stil_enable",	&xs_cfg.stilDBEnable,		0 },
{ WTYPE_TEXT,	CTYPE_STR,	"cfg_stil_dbpath",	&xs_cfg.stilDBPath,		0 },
{ WTYPE_TEXT,	CTYPE_STR,	"cfg_hvsc_path",	&xs_cfg.hvscPath,		0 },

{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_subctrl_none",	&xs_cfg.subsongControl,		XS_SSC_NONE },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_subctrl_seek",	&xs_cfg.subsongControl,		XS_SSC_SEEK },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_subctrl_popup",	&xs_cfg.subsongControl,		XS_SSC_POPUP },
{ WTYPE_BGROUP,	CTYPE_INT,	"cfg_subctrl_patch",	&xs_cfg.subsongControl,		XS_SSC_PATCH },

{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_detectmagic",	&xs_cfg.detectMagic,		0 },

{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_ftitle_override",	&xs_cfg.titleOverride,		0 },
{ WTYPE_TEXT,	CTYPE_STR,	"cfg_ftitle_format",	&xs_cfg.titleFormat,		0 },

{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_subauto_enable",	&xs_cfg.subAutoEnable,		0 },
{ WTYPE_BUTTON,	CTYPE_BOOL,	"cfg_subauto_min_only",	&xs_cfg.subAutoMinOnly,		0 },
{ WTYPE_SPIN,	CTYPE_INT,	"cfg_subauto_mintime",	&xs_cfg.subAutoMinTime,		0 },
};

static const gint xs_widtable_max = (sizeof(xs_widtable) / sizeof(t_xs_wid_item));


/*
 * Reset/initialize the configuration
 */
void xs_init_configuration(void)
{
	/* Lock configuration mutex */
	XSDEBUG("initializing configuration ...\n");
	g_static_mutex_lock(&xs_cfg_mutex);

	/* Initialize values with sensible defaults */
	xs_cfg.audioBitsPerSample = XS_RES_16BIT;
	xs_cfg.audioChannels = XS_CHN_MONO;
	xs_cfg.audioFrequency = 44100;

	xs_cfg.mos8580 = FALSE;
	xs_cfg.forceModel = FALSE;

	xs_cfg.emulateFilters = TRUE;
	xs_cfg.filterFs = XS_SIDPLAY1_FS;
	xs_cfg.filterFm = XS_SIDPLAY1_FM;
	xs_cfg.filterFt = XS_SIDPLAY1_FT;

#ifdef HAVE_SIDPLAY2
	xs_cfg.playerEngine = XS_ENG_SIDPLAY2;
	xs_cfg.memoryMode = XS_MPU_REAL;
#else
#ifdef HAVE_SIDPLAY1
	xs_cfg.playerEngine = XS_ENG_SIDPLAY1;
	xs_cfg.memoryMode = XS_MPU_BANK_SWITCHING;
#else
#error This should not happen! No emulator engines configured in!
#endif
#endif

	xs_cfg.clockSpeed = XS_CLOCK_PAL;
	xs_cfg.forceSpeed = FALSE;

	xs_cfg.sid2OptLevel = FALSE;
#ifdef HAVE_RESID_BUILDER
	xs_cfg.sid2Builder = XS_BLD_RESID;
#else
#ifdef HAVE_HARDSID_BUILDER
	xs_cfg.sid2Builder = XS_BLD_HARDSID;
#else
#ifdef HAVE_SIDPLAY2
#error This should not happen! No supported SIDPlay2 builders configured in!
#endif
#endif
#endif

	xs_cfg.oversampleEnable = FALSE;
	xs_cfg.oversampleFactor = XS_MIN_OVERSAMPLE;

	xs_cfg.playMaxTimeEnable = FALSE;
	xs_cfg.playMaxTimeUnknown = FALSE;
	xs_cfg.playMaxTime = 150;

	xs_cfg.playMinTimeEnable = FALSE;
	xs_cfg.playMinTime = 15;

	xs_cfg.songlenDBEnable = FALSE;
	xs_pstrcpy(&xs_cfg.songlenDBPath, "~/C64Music/Songlengths.txt");

	xs_cfg.stilDBEnable = FALSE;
	xs_pstrcpy(&xs_cfg.stilDBPath, "~/C64Music/DOCUMENTS/STIL.txt");
	xs_pstrcpy(&xs_cfg.hvscPath, "~/C64Music");

#ifdef HAVE_SONG_POSITION
	xs_cfg.subsongControl = XS_SSC_PATCH;
#else
	xs_cfg.subsongControl = XS_SSC_POPUP;
#endif

	xs_cfg.detectMagic = FALSE;

#ifdef HAVE_XMMSEXTRA
	xs_cfg.titleOverride = FALSE;
#else
	xs_cfg.titleOverride = TRUE;
#endif
	xs_pstrcpy(&xs_cfg.titleFormat, "%p - %t (%c) [%n/%N][%m]");


	xs_cfg.subAutoEnable = FALSE;
	xs_cfg.subAutoMinOnly = TRUE;
	xs_cfg.subAutoMinTime = 15;


	/* Unlock the configuration */
	g_static_mutex_unlock(&xs_cfg_mutex);
}


/*
 * Get the configuration (from file or default)
 */
void xs_read_configuration(void)
{
	gchar *tmpStr;
	ConfigDb *db;
	gint i;

	/* Try to open the configuration database */
	g_static_mutex_lock(&xs_cfg_mutex);
	XSDEBUG("loading from config-file ...\n");
	db = bmp_cfg_db_open();

	/* Read the new settings from configuration database */
	for (i = 0; i < xs_cfgtable_max; i++) {
		switch (xs_cfgtable[i].itemType) {
		case CTYPE_INT:
			bmp_cfg_db_get_int(db, XS_CONFIG_IDENT, xs_cfgtable[i].itemName,
					  (gint *) xs_cfgtable[i].itemData);
			break;

		case CTYPE_BOOL:
			bmp_cfg_db_get_bool(db, XS_CONFIG_IDENT, xs_cfgtable[i].itemName,
					      (gboolean *) xs_cfgtable[i].itemData);
			break;

		case CTYPE_FLOAT:
			bmp_cfg_db_get_float(db, XS_CONFIG_IDENT, xs_cfgtable[i].itemName,
					    (gfloat *) xs_cfgtable[i].itemData);
			break;

		case CTYPE_STR:
			if (bmp_cfg_db_get_string
			    (db, XS_CONFIG_IDENT, xs_cfgtable[i].itemName, (gchar **) & tmpStr)) {
				/* Read was successfull */
				xs_pstrcpy((gchar **) xs_cfgtable[i].itemData, tmpStr);
				g_free(tmpStr);
			}
			break;

		default:
			XSERR
			    ("Internal: Unsupported setting type found while reading configuration file. Please report to author!\n");
			break;
		}
	}

	bmp_cfg_db_close(db);

	g_static_mutex_unlock(&xs_cfg_mutex);
	XSDEBUG("OK\n");
}



/*
 * Write the current configuration
 */
gint xs_write_configuration(void)
{
	ConfigDb *db;
	gint i;

	XSDEBUG("writing configuration ...\n");
	g_static_mutex_lock(&xs_cfg_mutex);
	db = bmp_cfg_db_open();

	/* Write the new settings to configuration database */
	for (i = 0; i < xs_cfgtable_max; i++) {
		switch (xs_cfgtable[i].itemType) {
		case CTYPE_INT:
			bmp_cfg_db_set_int(db, XS_CONFIG_IDENT, xs_cfgtable[i].itemName,
					   *(gint *) xs_cfgtable[i].itemData);
			break;

		case CTYPE_BOOL:
			bmp_cfg_db_set_bool(db, XS_CONFIG_IDENT, xs_cfgtable[i].itemName,
					       *(gboolean *) xs_cfgtable[i].itemData);
			break;

		case CTYPE_FLOAT:
			bmp_cfg_db_set_float(db, XS_CONFIG_IDENT, xs_cfgtable[i].itemName,
					     *(gfloat *) xs_cfgtable[i].itemData);
			break;

		case CTYPE_STR:
			bmp_cfg_db_set_string(db, XS_CONFIG_IDENT, xs_cfgtable[i].itemName,
					      *(gchar **) xs_cfgtable[i].itemData);
			break;

		default:
			XSERR
			    ("Internal: Unsupported setting type found while writing configuration file. Please report to author!\n");
			break;
		}
	}

	bmp_cfg_db_close(db);

	g_static_mutex_unlock(&xs_cfg_mutex);

	return 0;
}


/*
 * Configuration panel was canceled
 */
void xs_cfg_cancel(void)
{
	gtk_widget_destroy(xs_configwin);
	xs_configwin = NULL;
}


/*
 * Configuration was accepted (OK), save the settings
 */
void xs_cfg_ok(void)
{
	gint i;
	gfloat tmpValue;

	XSDEBUG("get data from widgets to config...\n");

	for (i = 0; i < xs_widtable_max; i++) {
		switch (xs_widtable[i].widType) {
		case WTYPE_BGROUP:
			/* Check if toggle-button is active */
			if (GTK_TOGGLE_BUTTON(LUW(xs_widtable[i].widName))->active) {
				/* Yes, set the constant value */
				*((gint *) xs_widtable[i].itemData) = xs_widtable[i].itemSet;
			}
			break;

		case WTYPE_SPIN:
		case WTYPE_SCALE:
			/* Get the value */
			switch (xs_widtable[i].widType) {
			case WTYPE_SPIN:
				tmpValue =
				    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(LUW(xs_widtable[i].widName)))->value;
				break;

			case WTYPE_SCALE:
				tmpValue = gtk_range_get_adjustment(GTK_RANGE(LUW(xs_widtable[i].widName)))->value;
				break;

			default:
				tmpValue = -1;
			}

			/* Set the value */
			switch (xs_widtable[i].itemType) {
			case CTYPE_INT:
				*((gint *) xs_widtable[i].itemData) = (gint) tmpValue;
				break;

			case CTYPE_FLOAT:
				*((gfloat *) xs_widtable[i].itemData) = tmpValue;
				break;
			}
			break;

		case WTYPE_BUTTON:
			/* Check if toggle-button is active */
			*((gboolean *) xs_widtable[i].itemData) =
			    (GTK_TOGGLE_BUTTON(LUW(xs_widtable[i].widName))->active);
			break;

		case WTYPE_TEXT:
			/* Get text from text-widget */
			xs_pstrcpy((gchar **) xs_widtable[i].itemData,
				   gtk_entry_get_text(GTK_ENTRY(LUW(xs_widtable[i].widName)))
			    );
			break;
		}
	}

	/* Close window */
	gtk_widget_destroy(xs_configwin);
	xs_configwin = NULL;

	/* Write settings */
	xs_write_configuration();

	/* Re-initialize */
	xs_reinit();
}


/*
 * Reset filter settings to defaults
 */
void xs_cfg_filter_reset(GtkButton * button, gpointer user_data)
{
	(void) button;
	(void) user_data;

	gtk_adjustment_set_value(gtk_range_get_adjustment(GTK_RANGE(LUW("cfg_emu_filt_fs"))), XS_SIDPLAY1_FS);
	gtk_adjustment_set_value(gtk_range_get_adjustment(GTK_RANGE(LUW("cfg_emu_filt_fm"))), XS_SIDPLAY1_FM);
	gtk_adjustment_set_value(gtk_range_get_adjustment(GTK_RANGE(LUW("cfg_emu_filt_ft"))), XS_SIDPLAY1_FT);
}




/*
 * HVSC songlength-database file selector response-functions
 */
void xs_cfg_sld_dbbrowse(GtkButton * button, gpointer user_data)
{
	(void) button;
	(void) user_data;

	if (xs_sldb_fileselector != NULL) {
		gdk_window_raise(xs_sldb_fileselector->window);
		return;
	}

	xs_sldb_fileselector = create_xs_sldbfileselector();
	g_static_mutex_lock(&xs_cfg_mutex);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(xs_sldb_fileselector), xs_cfg.songlenDBPath);
	g_static_mutex_unlock(&xs_cfg_mutex);
	gtk_widget_show(xs_sldb_fileselector);
}


void xs_cfg_sldb_fs_ok(void)
{
	/* Selection was accepted! */
	gtk_entry_set_text(GTK_ENTRY(LUW("cfg_sld_dbpath")),
			   gtk_file_selection_get_filename(GTK_FILE_SELECTION(xs_sldb_fileselector)));

	/* Close file selector window */
	gtk_widget_destroy(xs_sldb_fileselector);
	xs_sldb_fileselector = NULL;
}


void xs_cfg_sldb_fs_cancel(void)
{
	/* Close file selector window */
	gtk_widget_destroy(xs_sldb_fileselector);
	xs_sldb_fileselector = NULL;
}


/*
 * STIL-database file selector response-functions
 */
void xs_cfg_stil_browse(GtkButton * button, gpointer user_data)
{
	(void) button;
	(void) user_data;

	if (xs_stil_fileselector != NULL) {
		gdk_window_raise(xs_stil_fileselector->window);
		return;
	}

	xs_stil_fileselector = create_xs_stilfileselector();
	g_static_mutex_lock(&xs_cfg_mutex);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(xs_stil_fileselector), xs_cfg.stilDBPath);
	g_static_mutex_unlock(&xs_cfg_mutex);
	gtk_widget_show(xs_stil_fileselector);
}


void xs_cfg_stil_fs_ok(void)
{
	/* Selection was accepted! */
	gtk_entry_set_text(GTK_ENTRY(LUW("cfg_stil_dbpath")),
			   gtk_file_selection_get_filename(GTK_FILE_SELECTION(xs_stil_fileselector)));

	/* Close file selector window */
	gtk_widget_destroy(xs_stil_fileselector);
	xs_stil_fileselector = NULL;
}


void xs_cfg_stil_fs_cancel(void)
{
	/* Close file selector window */
	gtk_widget_destroy(xs_stil_fileselector);
	xs_stil_fileselector = NULL;
}


/*
 * HVSC location selector response-functions
 */
void xs_cfg_hvsc_browse(GtkButton * button, gpointer user_data)
{
	(void) button;
	(void) user_data;

	if (xs_hvsc_pathselector != NULL) {
		gdk_window_raise(xs_hvsc_pathselector->window);
		return;
	}

	xs_hvsc_pathselector = create_xs_hvscpathselector();
	g_static_mutex_lock(&xs_cfg_mutex);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(xs_hvsc_pathselector), xs_cfg.hvscPath);
	g_static_mutex_unlock(&xs_cfg_mutex);
	gtk_widget_show(xs_hvsc_pathselector);
}


void xs_cfg_hvsc_fs_ok(void)
{
	/* Selection was accepted! */
	gtk_entry_set_text(GTK_ENTRY(LUW("cfg_hvsc_path")),
			   gtk_file_selection_get_filename(GTK_FILE_SELECTION(xs_hvsc_pathselector)));

	/* Close file selector window */
	gtk_widget_destroy(xs_hvsc_pathselector);
	xs_hvsc_pathselector = NULL;
}


void xs_cfg_hvsc_fs_cancel(void)
{
	/* Close file selector window */
	gtk_widget_destroy(xs_hvsc_pathselector);
	xs_hvsc_pathselector = NULL;
}


/*
 * Selection toggle handlers
 */
void xs_cfg_emu_filters_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_filters_notebook"), isActive);
}


void xs_cfg_ftitle_override_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_ftitle_format"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_ftitle_desc1"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_ftitle_desc2"), isActive);
}


void xs_cfg_emu_sidplay1_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	(void) togglebutton;
	(void) user_data;
}


void xs_cfg_emu_sidplay2_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_emu_mem_real"), isActive);

	gtk_widget_set_sensitive(LUW("cfg_sidplay2_grp"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_emu_sidplay2_opt"), isActive);

	gtk_widget_set_sensitive(LUW("cfg_chn_autopan"), !isActive);

#ifdef HAVE_RESID_BUILDER
	gtk_widget_set_sensitive(LUW("cfg_emu_sidplay2_resid"), isActive);
#else
	gtk_widget_set_sensitive(LUW("cfg_emu_sidplay2_resid"), FALSE);
#endif

#ifdef HAVE_HARDSID_BUILDER
	gtk_widget_set_sensitive(LUW("cfg_emu_sidplay2_hardsid"), isActive);
#else
	gtk_widget_set_sensitive(LUW("cfg_emu_sidplay2_hardsid"), FALSE);
#endif
}


void xs_cfg_oversample_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_oversample_factor"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_oversample_label1"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_oversample_label2"), isActive);
}


void xs_cfg_mintime_enable_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_mintime"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_mintime_label1"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_mintime_label2"), isActive);
}


void xs_cfg_maxtime_enable_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(LUW("cfg_maxtime_enable"))->active;

	(void) togglebutton;
	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_maxtime_unknown"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_maxtime"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_maxtime_label1"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_maxtime_label2"), isActive);
}


void xs_cfg_sld_enable_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_sld_dbpath"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_sld_dbbrowse"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_sld_label1"), isActive);
}


void xs_cfg_stil_enable_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_stil_dbpath"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_stil_browse"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_stil_label1"), isActive);

	gtk_widget_set_sensitive(LUW("cfg_hvsc_path"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_hvsc_browse"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_hvsc_label1"), isActive);
}


void xs_cfg_subauto_enable_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_subauto_min_only"), isActive);
	gtk_widget_set_sensitive(LUW("cfg_subauto_mintime"), isActive);
}


void xs_cfg_subauto_min_only_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean isActive = GTK_TOGGLE_BUTTON(togglebutton)->active &&
		GTK_TOGGLE_BUTTON(LUW("cfg_subauto_enable"))->active;

	(void) user_data;

	gtk_widget_set_sensitive(LUW("cfg_subauto_mintime"), isActive);
}


void xs_cfg_mintime_changed(GtkEditable * editable, gpointer user_data)
{
	gint tmpValue;
	GtkAdjustment *tmpAdj;

	(void) user_data;

	tmpAdj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(LUW("cfg_maxtime")));

	tmpValue = (gint) gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(editable))->value;

	if (tmpValue > tmpAdj->value)
		gtk_adjustment_set_value(tmpAdj, tmpValue);
}


void xs_cfg_maxtime_changed(GtkEditable * editable, gpointer user_data)
{
	gint tmpValue;
	GtkAdjustment *tmpAdj;

	(void) user_data;

	tmpAdj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(LUW("cfg_mintime")));

	tmpValue = (gint) gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(editable))->value;

	if (tmpValue < tmpAdj->value)
		gtk_adjustment_set_value(tmpAdj, tmpValue);
}


void xs_cfg_filter2_reset(GtkButton * button, gpointer user_data)
{
	(void) button; (void) user_data;
}

/*
 * Execute the configuration panel
 */
void xs_configure(void)
{
	gint i;
	gfloat tmpValue;

	/* Check if the window already exists */
	if (xs_configwin != NULL) {
		gdk_window_raise(xs_configwin->window);
		return;
	}

	/* Create the window */
	xs_configwin = create_xs_configwin();

	/* Get lock on configuration */
	g_static_mutex_lock(&xs_cfg_mutex);

	/* Based on available optional parts, gray out options */
#ifndef HAVE_SIDPLAY1
	gtk_widget_set_sensitive(LUW("cfg_emu_sidplay1"), FALSE);
	gtk_widget_set_sensitive(LUW("cfg_box_sidplay1"), FALSE);
#endif

#ifndef HAVE_SIDPLAY2
	gtk_widget_set_sensitive(LUW("cfg_emu_sidplay2"), FALSE);
	gtk_widget_set_sensitive(LUW("cfg_box_sidplay2"), FALSE);
#endif

#ifndef HAVE_XMMSEXTRA
	gtk_widget_set_sensitive(LUW("cfg_ftitle_override"), FALSE);
	xs_cfg.titleOverride = TRUE;
#endif

#ifndef HAVE_SONG_POSITION
	gtk_widget_set_sensitive(LUW("cfg_subctrl_patch"), FALSE);
#endif

	/* Update the widget sensitivities */
	xs_cfg_emu_filters_toggled((GtkToggleButton *) LUW("cfg_emu_filters"), NULL);
	xs_cfg_ftitle_override_toggled((GtkToggleButton *) LUW("cfg_ftitle_override"), NULL);
	xs_cfg_emu_sidplay1_toggled((GtkToggleButton *) LUW("cfg_emu_sidplay1"), NULL);
	xs_cfg_emu_sidplay2_toggled((GtkToggleButton *) LUW("cfg_emu_sidplay2"), NULL);
	xs_cfg_oversample_toggled((GtkToggleButton *) LUW("cfg_oversample"), NULL);
	xs_cfg_mintime_enable_toggled((GtkToggleButton *) LUW("cfg_mintime_enable"), NULL);
	xs_cfg_maxtime_enable_toggled((GtkToggleButton *) LUW("cfg_maxtime_enable"), NULL);
	xs_cfg_sld_enable_toggled((GtkToggleButton *) LUW("cfg_sld_enable"), NULL);
	xs_cfg_stil_enable_toggled((GtkToggleButton *) LUW("cfg_stil_enable"), NULL);
	xs_cfg_subauto_enable_toggled((GtkToggleButton *) LUW("cfg_subauto_enable"), NULL);
	xs_cfg_subauto_min_only_toggled((GtkToggleButton *) LUW("cfg_subauto_min_only"), NULL);


	/* Set current data to widgets */
	for (i = 0; i < xs_widtable_max; i++) {
		switch (xs_widtable[i].widType) {
		case WTYPE_BGROUP:
			/* Check if current value matches the given one */
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(LUW(xs_widtable[i].widName)),
						     (*((gint *) xs_widtable[i].itemData) == xs_widtable[i].itemSet)
			    );
			break;


		case WTYPE_SPIN:
		case WTYPE_SCALE:
			/* Get the value */
			switch (xs_widtable[i].itemType) {
			case CTYPE_INT:
				tmpValue = (gfloat) * ((gint *) xs_widtable[i].itemData);
				break;

			case CTYPE_FLOAT:
				tmpValue = *((gfloat *) xs_widtable[i].itemData);
				break;

			default:
				tmpValue = -1;
			}

			/* Set the value */
			switch (xs_widtable[i].widType) {
			case WTYPE_SPIN:
				gtk_adjustment_set_value(gtk_spin_button_get_adjustment
							 (GTK_SPIN_BUTTON(LUW(xs_widtable[i].widName))), tmpValue);
				break;

			case WTYPE_SCALE:
				gtk_adjustment_set_value(gtk_range_get_adjustment
							 (GTK_RANGE(LUW(xs_widtable[i].widName))), tmpValue);
				break;
			}
			break;

		case WTYPE_BUTTON:
			/* Set toggle-button */
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(LUW(xs_widtable[i].widName)),
						     *((gboolean *) xs_widtable[i].itemData)
			    );
			break;

		case WTYPE_TEXT:
			/* Set text to text-widget */
			if (*(gchar **) xs_widtable[i].itemData != NULL) {
				gtk_entry_set_text(GTK_ENTRY(LUW(xs_widtable[i].widName)),
						   *(gchar **) xs_widtable[i].itemData);
			}
			break;
		}
	}

	/* Release the configuration */
	g_static_mutex_unlock(&xs_cfg_mutex);

	/* Show the widget */
	gtk_widget_show(xs_configwin);
}
