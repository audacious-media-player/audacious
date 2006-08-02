/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */
#ifndef CDAUDIO_H
#define CDAUDIO_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include "audacious/plugin.h"

#include "cdinfo.h"

#ifdef HAVE_OSS
#include <Output/OSS/soundcard.h>
#endif

#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif

#ifdef HAVE_GETMNTINFO
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif

#ifndef CD_FRAMES
#define CD_FRAMES 75
#endif

#include <sys/types.h>

#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#elif defined HAVE_SYS_CDIO_H
#include <sys/cdio.h>
#include <sys/cdrio.h>
#endif

#if defined(CDROMREADAUDIO) || defined(CDIOCREADAUDIO) || defined(CDROMCDDA) || defined(CDRIOCSETBLOCKSIZE)
# define CDDA_HAS_READAUDIO
#endif

#ifndef CD_FRAMESIZE_RAW
# define CD_FRAMESIZE_RAW 2352
#endif

/* Number of frames that are read at once in dae mode */
#define CDDA_DAE_FRAMES 8

#ifndef CDDA_HAS_READAUDIO
#warning "Digital audio extraction has not been ported to this platform"
#define read_audio_data(fd, pos, num, buf) -1
#else
int read_audio_data(int fd, int pos, int num, void *buf);
#endif


#ifdef __FreeBSD__
/*
 * FreeBSD won't be able to detect media changes if using O_NONBLOCK
 */
#define CDOPENFLAGS O_RDONLY
#else
#define CDOPENFLAGS (O_RDONLY | O_NONBLOCK)
#endif


#define CDDB_DEFAULT_SERVER "freedb.freedb.org"

struct driveinfo {
    gchar *device, *directory;
    gint mixer, oss_mixer;
    gboolean valid;
    gint dae;
};

typedef struct {
    GList *drives;

    gchar *cddb_server;
    gint cddb_protocol_level;
    gboolean use_cddb;

    gchar *cdin_server;
    gboolean use_cdin;

    gboolean title_override;
    char *name_format;
} CDDAConfig;

struct cdda_msf {
    guint8 minute;
    guint8 second;
    guint8 frame;
    struct {
        guint data_track:1;
    } flags;
};

/*
 * Note: This macro will convert to a LBA representation of the MSF
 * address, not to a true LBA address, as we don't subtract the offset
 */
#define LBA(msf) ((msf.minute * 60 + msf.second) * 75 + msf.frame)

#define CDDA_MSF_OFFSET 150

typedef struct {
    guint8 first_track, last_track;
    struct cdda_msf leadout;
    struct cdda_msf track[100];
} cdda_disc_toc_t;

extern CDDAConfig cdda_cfg;

enum {
    CDDA_MIXER_NONE,
    CDDA_MIXER_DRIVE,
    CDDA_MIXER_OSS,
};

enum {
    CDDA_READ_ANALOG,
    CDDA_READ_DAE,
};

void cdda_configure(void);
gboolean cdda_get_toc(cdda_disc_toc_t * info, const gchar *device);
guint32 cdda_cddb_compute_discid(cdda_disc_toc_t * info);
void cdda_cddb_get_info(cdda_disc_toc_t * toc, cdinfo_t * info);
void cdda_cdindex_get_idx(cdda_disc_toc_t * toc, cdinfo_t * cdinfo);
struct driveinfo *cdda_find_drive(gchar *filename);

void cdda_cddb_show_server_dialog(GtkWidget * w, gpointer data);
void cdda_cddb_set_server(const gchar *new_server);
void cddb_quit(void);

#endif
