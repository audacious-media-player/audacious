/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen <havardk@xmms.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "cdaudio.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <libaudacious/configdb.h>
#include <libaudacious/util.h>
#include <libaudacious/titlestring.h>
#include "audacious/output.h"

#ifdef CDROMSTOP
# define XMMS_STOP CDROMSTOP
#elif defined CDIOCSTOP
# define XMMS_STOP CDIOCSTOP
#else
# error "No stop ioctl"
#endif

#ifdef CDIOCPAUSE
# define XMMS_PAUSE CDIOCPAUSE
#elif defined CDROMPAUSE
# define XMMS_PAUSE CDROMPAUSE
#else
# error "No pause ioctl"
#endif

#ifdef CDIOCRESUME
# define XMMS_RESUME CDIOCRESUME
#elif defined CDROMRESUME
# define XMMS_RESUME CDROMRESUME
#else
# error "No resume ioctl"
#endif

/*
 * Distributions should not patch this, but instead use the
 * --with-cdda-device=path and --with-cdda-dir=path configure options.
 */

#ifndef CDDA_DEVICE
# ifdef HAVE_SYS_CDIO_H
#  if defined(__FreeBSD__) && !defined(CDIOCREADAUDIO)
#   define CDDA_DEVICE "/dev/acd0c"
#  elif defined __FreeBSD__
#   define CDDA_DEVICE "/dev/acd0"
#  elif defined __OpenBSD__
#   define CDDA_DEVICE "/dev/cd0c"
#  else
#   define CDDA_DEVICE "/vol/dev/aliases/cdrom0"
#  endif
# else
#   define CDDA_DEVICE "/dev/cdrom"
# endif
#endif

#ifndef CDDA_DIRECTORY
# ifdef HAVE_SYS_CDIO_H
#  ifdef __FreeBSD__
#   define CDDA_DIRECTORY "/cdrom"
#  elif defined __OpenBSD__
#   define CDDA_DIRECTORY "/cdrom"
#  else
#   define CDDA_DIRECTORY "/cdrom/cdrom0"
#  endif
# else
#   define CDDA_DIRECTORY "/mnt/cdrom"
# endif
#endif




static TitleInput *cdda_get_tuple(cdda_disc_toc_t * toc, int track);
static gchar *get_song_title(TitleInput *tuple);
static gboolean stop_timeout(gpointer data);

static void cdda_init(void);
static int is_our_file(char *filename);
static GList *scan_dir(char *dir);
static void play_file(char *filename);
static void stop(void);
static void cdda_pause(short p);
static void seek(int time);
static int get_time(void);
static void get_song_info(char *filename, char **title, int *length);
static TitleInput *get_song_tuple(char *filename);
static void get_volume(int *l, int *r);
static void set_volume(int l, int r);
static void cleanup(void);
void cdda_fileinfo(char *filename);

InputPlugin cdda_ip = {
    NULL,
    NULL,
    NULL,                       /* Description */
    cdda_init,
    NULL,                       /* about */
    cdda_configure,
    is_our_file,
    scan_dir,
    play_file,
    stop,
    cdda_pause,
    seek,
    NULL,                       /* set_eq */
    get_time,
    get_volume,
    set_volume,
    cleanup,
    NULL,                       /* obsolete */
    NULL,                       /* add_vis_pcm */
    NULL,                       /* set_info, filled in by xmms */
    NULL,                       /* set_info_text, filled in by xmms */
    get_song_info,
    NULL,                       /*  cdda_fileinfo, *//* file_info_box */
    NULL,                        /* output plugin handle */
    get_song_tuple
};

CDDAConfig cdda_cfg;

static struct {
    struct driveinfo drive;
    cdda_disc_toc_t cd_toc;
    int track;
    int fd;
    gboolean playing;
} cdda_playing;

static struct {
    GThread *thread;
    gboolean audio_error, eof;
    int seek;

} dae_data;

static gboolean is_paused;
static int pause_time;

struct timeout {
    int id;
    char *device;
};

static GList *timeout_list;

/* Time to delay stop command in 1/10 second */
#define STOP_DELAY 20

InputPlugin *
get_iplugin_info(void)
{
    cdda_ip.description = g_strdup_printf(_("CD Audio Plugin"));
    return &cdda_ip;
}



#ifdef BEEP_CDROM_SOLARIS
/*
 * Lowlevel cdrom access, Solaris style (Solaris, Linux)
 */

static void
play_ioctl(struct cdda_msf *start, struct cdda_msf *end)
{
    struct cdrom_msf msf;

    msf.cdmsf_min0 = start->minute;
    msf.cdmsf_sec0 = start->second;
    msf.cdmsf_frame0 = start->frame;
    msf.cdmsf_min1 = end->minute;
    msf.cdmsf_sec1 = end->second;
    msf.cdmsf_frame1 = end->frame;
    ioctl(cdda_playing.fd, CDROMPLAYMSF, &msf);
}

static int
get_current_frame(void)
{
    struct cdrom_subchnl subchnl;

    subchnl.cdsc_format = CDROM_MSF;
    if (ioctl(cdda_playing.fd, CDROMSUBCHNL, &subchnl) < 0)
        return -1;

    switch (subchnl.cdsc_audiostatus) {
    case CDROM_AUDIO_COMPLETED:
    case CDROM_AUDIO_ERROR:
        return -1;
    }

    return (LBA(subchnl.cdsc_absaddr.msf));
}

#if !defined(CDROMVOLREAD)
static int volume_left = 100, volume_right = 100;
#endif

static void
drive_get_volume(int *l, int *r)
{
#if defined(CDROMVOLREAD)
    struct cdrom_volctrl vol;

    if (cdda_playing.fd != -1 && !ioctl(cdda_playing.fd, CDROMVOLREAD, &vol)) {
        *l = (100 * vol.channel0) / 255;
        *r = (100 * vol.channel1) / 255;
    }
#if 0
    else if (cdda_playing.fd != -1)
        g_message("CDROMVOLREAD failed");
#endif
#else
    *l = volume_left;
    *r = volume_right;
#endif
}

static void
drive_set_volume(int l, int r)
{
    struct cdrom_volctrl vol;

    if (cdda_playing.fd != -1) {
        vol.channel0 = vol.channel2 = (l * 255) / 100;
        vol.channel1 = vol.channel3 = (r * 255) / 100;
        ioctl(cdda_playing.fd, CDROMVOLCTRL, &vol);
    }
#if !defined(CDROMVOLREAD)
    volume_left = l;
    volume_right = r;
#endif
}

#ifdef CDROMREADAUDIO
int
read_audio_data(int fd, int pos, int num, void *buf)
{
    struct cdrom_read_audio cdra;

#if 1
    cdra.addr.lba = pos - CDDA_MSF_OFFSET;
    cdra.addr_format = CDROM_LBA;
#else
    cdra.addr.msf.minute = pos / (60 * 75);
    cdra.addr.msf.second = (pos / 75) % 60;
    cdra.addr.msf.frame = pos % 75;
    cdra.addr_format = CDROM_MSF;
#endif

    cdra.nframes = num;
    cdra.buf = buf;

    if (ioctl(fd, CDROMREADAUDIO, &cdra) < 0)
        return -errno;

    return cdra.nframes;
}
#endif                          /* CDROMREADAUDIO */

#if defined(CDROMCDDA)
int
read_audio_data(int fd, int pos, int num, void *buf)
{
    struct cdrom_cdda cdra;

    cdra.cdda_addr = pos - CDDA_MSF_OFFSET;
    cdra.cdda_length = num;
    cdra.cdda_data = buf;
    cdra.cdda_subcode = CDROM_DA_NO_SUBCODE;
    if (ioctl(fd, CDROMCDDA, &cdra) < 0)
        return -errno;

    return cdra.cdda_length;
}
#endif

static gboolean
cdda_get_toc_lowlevel(int fd, cdda_disc_toc_t * info)
{
    struct cdrom_tochdr tochdr;
    struct cdrom_tocentry tocentry;
    int i;



    if (ioctl(fd, CDROMREADTOCHDR, &tochdr))
        return FALSE;

    for (i = tochdr.cdth_trk0; i <= tochdr.cdth_trk1; i++) {
        tocentry.cdte_format = CDROM_MSF;
        tocentry.cdte_track = i;
        if (ioctl(fd, CDROMREADTOCENTRY, &tocentry))
            return FALSE;
        info->track[i].minute = tocentry.cdte_addr.msf.minute;
        info->track[i].second = tocentry.cdte_addr.msf.second;
        info->track[i].frame = tocentry.cdte_addr.msf.frame;
        info->track[i].flags.data_track =
            tocentry.cdte_ctrl == CDROM_DATA_TRACK;

    }

    /* Get the leadout track */
    tocentry.cdte_track = CDROM_LEADOUT;
    tocentry.cdte_format = CDROM_MSF;

    if (ioctl(fd, CDROMREADTOCENTRY, &tocentry))
        return FALSE;
    info->leadout.minute = tocentry.cdte_addr.msf.minute;
    info->leadout.second = tocentry.cdte_addr.msf.second;
    info->leadout.frame = tocentry.cdte_addr.msf.frame;

    info->first_track = tochdr.cdth_trk0;
    info->last_track = tochdr.cdth_trk1;

    return TRUE;
}

#endif

#ifdef BEEP_CDROM_BSD
/*
 * Lowlevel cdrom access, BSD style (FreeBSD, OpenBSD, NetBSD, Darwin)
 */

static void
play_ioctl(struct cdda_msf *start, struct cdda_msf *end)
{
    struct ioc_play_msf msf;

    msf.start_m = start->minute;
    msf.start_s = start->second;
    msf.start_f = start->frame;
    msf.end_m = end->minute;
    msf.end_s = end->second;
    msf.end_f = end->frame;
    ioctl(cdda_playing.fd, CDIOCPLAYMSF, &msf);
}

static int
get_current_frame(void)
{
    struct ioc_read_subchannel subchnl;
    struct cd_sub_channel_info subinfo;
    subchnl.address_format = CD_MSF_FORMAT;
    subchnl.data_format = CD_CURRENT_POSITION;
    subchnl.track = 0;
    subchnl.data_len = sizeof(subinfo);
    subchnl.data = &subinfo;
    if (ioctl(cdda_playing.fd, CDIOCREADSUBCHANNEL, &subchnl) < 0)
        return -1;

#ifdef BEEP_CDROM_BSD_DARWIN
    return ((subchnl.data->what.position.absaddr[1] * 60
             subchnl.data->what.position.absaddr[2]) * 75 +
            subchnl.data->what.position.absaddr[3]);
#else
    return (LBA(subchnl.data->what.position.absaddr.msf));
#endif
}

static void
drive_get_volume(int *l, int *r)
{
    struct ioc_vol vol;

    if (cdda_playing.fd != -1) {
        ioctl(cdda_playing.fd, CDIOCGETVOL, &vol);
        *l = (100 * vol.vol[0]) / 255;
        *r = (100 * vol.vol[1]) / 255;
    }
}

static void
drive_set_volume(int l, int r)
{
    struct ioc_vol vol;

    if (cdda_playing.fd != -1) {
        vol.vol[0] = vol.vol[2] = (l * 255) / 100;
        vol.vol[1] = vol.vol[3] = (r * 255) / 100;
        ioctl(cdda_playing.fd, CDIOCSETVOL, &vol);
    }
}

#if defined(__FreeBSD__) && !defined(CDIOCREADAUDIO)
int
read_audio_data(int fd, int pos, int num, void *buf)
{
    int bs = CD_FRAMESIZE_RAW;

    if (ioctl(fd, CDRIOCSETBLOCKSIZE, &bs) == -1)
	return -1;
    if (pread(fd, buf, num * bs, (pos - 150) * bs) != num * bs)
	return -1;

    return num;
}
#endif

#if defined(CDIOCREADAUDIO)
int
read_audio_data(int fd, int pos, int num, void *buf)
{
    struct ioc_read_audio cdra;

    cdra.address.lba = pos - CDDA_MSF_OFFSET;
    cdra.address_format = CD_LBA_FORMAT;
    cdra.nframes = num;
    cdra.buffer = buf;

    if (ioctl(fd, CDIOCREADAUDIO, &cdra) < 0)
        return -errno;

    return cdra.nframes;
}
#endif                          /* CDIOCREADAUDIO */

#ifdef BEEP_CDROM_BSD_NETBSD    /* NetBSD, OpenBSD */

static gboolean
cdda_get_toc_lowlevel(int fd, cdda_disc_toc_t * info)
{
    struct ioc_toc_header tochdr;
    struct ioc_read_toc_entry tocentry;
    struct cd_toc_entry tocentrydata;
    int i;

    if (ioctl(fd, CDIOREADTOCHEADER, &tochdr))
        return FALSE;

    for (i = tochdr.starting_track; i <= tochdr.ending_track; i++) {
        tocentry.address_format = CD_MSF_FORMAT;

        tocentry.starting_track = i;
        tocentry.data = &tocentrydata;
        tocentry.data_len = sizeof(tocentrydata);
        if (ioctl(fd, CDIOREADTOCENTRYS, &tocentry))
            return FALSE;
        info->track[i].minute = tocentry.data->addr.msf.minute;
        info->track[i].second = tocentry.data->addr.msf.second;
        info->track[i].frame = tocentry.data->addr.msf.frame;
        info->track[i].flags.data_track = (tocentry.data->control & 4) == 4;
    }

    /* Get the leadout track */
    tocentry.address_format = CD_MSF_FORMAT;

    tocentry.starting_track = 0xAA;
    tocentry.data = &tocentrydata;
    tocentry.data_len = sizeof(tocentrydata);
    if (ioctl(fd, CDIOREADTOCENTRYS, &tocentry))
        return FALSE;
    info->leadout.minute = tocentry.data->addr.msf.minute;
    info->leadout.second = tocentry.data->addr.msf.second;
    info->leadout.frame = tocentry.data->addr.msf.frame;

    info->first_track = tochdr.starting_track;
    info->last_track = tochdr.ending_track;

    return TRUE;
}

#elif defined(BEEP_CDROM_BSD_DARWIN)

static gboolean
cdda_get_toc_lowlevel(int fd, cdda_disc_toc_t * info)
{
    struct ioc_toc_header tochdr;
    struct ioc_read_toc_entry tocentry;
    int i;

    if (ioctl(fd, CDIOREADTOCHEADER, &tochdr))
        return FALSE;

    for (i = tochdr.starting_track; i <= tochdr.ending_track; i++) {
        tocentry.address_format = CD_MSF_FORMAT;

        tocentry.starting_track = i;
        if (ioctl(fd, CDIOREADTOCENTRYS, &tocentry))
            return FALSE;
        info->track[i].minute = tocentry.data->addr[1];
        info->track[i].second = tocentry.data->addr[2];
        info->track[i].frame = tocentry.data->addr[3];
        info->track[i].flags.data_track = (tocentry.data->control & 4) == 4;
    }

    /* Get the leadout track */
    tocentry.address_format = CD_MSF_FORMAT;

    tocentry.starting_track = 0xAA;
    if (ioctl(fd, CDIOREADTOCENTRYS, &tocentry))
        return FALSE;
    info->leadout.minute = tocentry.data->addr[1];
    info->leadout.second = tocentry.data->addr[2];
    info->leadout.frame = tocentry.data->addr[3];

    return TRUE;
}

#else                           /* FreeBSD */

static gboolean
cdda_get_toc_lowlevel(int fd, cdda_disc_toc_t * info)
{
    struct ioc_toc_header tochdr;
    struct ioc_read_toc_single_entry tocentry;
    int i;

    if (ioctl(fd, CDIOREADTOCHEADER, &tochdr))
        return FALSE;

    for (i = tochdr.starting_track; i <= tochdr.ending_track; i++) {
        tocentry.address_format = CD_MSF_FORMAT;

        tocentry.track = i;
        if (ioctl(fd, CDIOREADTOCENTRY, &tocentry))
            return FALSE;
        info->track[i].minute = tocentry.entry.addr.msf.minute;
        info->track[i].second = tocentry.entry.addr.msf.second;
        info->track[i].frame = tocentry.entry.addr.msf.frame;
        info->track[i].flags.data_track = (tocentry.entry.control & 4) == 4;
    }

    /* Get the leadout track */
    tocentry.address_format = CD_MSF_FORMAT;

    tocentry.track = 0xAA;
    if (ioctl(fd, CDIOREADTOCENTRY, &tocentry))
        return FALSE;
    info->leadout.minute = tocentry.entry.addr.msf.minute;
    info->leadout.second = tocentry.entry.addr.msf.second;
    info->leadout.frame = tocentry.entry.addr.msf.frame;

    info->first_track = tochdr.starting_track;
    info->last_track = tochdr.ending_track;

    return TRUE;
}
#endif

#endif











extern gboolean
is_mounted(const char *device_name)
{
#if defined(HAVE_MNTENT_H) || defined(HAVE_GETMNTINFO)
    char devname[256];
    struct stat st;
#if defined(HAVE_MNTENT_H)
    FILE *mounts;
    struct mntent *mnt;
#elif defined(HAVE_GETMNTINFO)
    struct statfs *fsp;
    int entries;
#endif

    if (lstat(device_name, &st) < 0)
        return -1;

    if (S_ISLNK(st.st_mode))
        readlink(device_name, devname, 256);
    else
        strncpy(devname, device_name, 256);

#if defined(HAVE_MNTENT_H)
    if ((mounts = setmntent(MOUNTED, "r")) == NULL)
        return TRUE;

    while ((mnt = getmntent(mounts)) != NULL) {
        if (strcmp(mnt->mnt_fsname, devname) == 0) {
            endmntent(mounts);
            return TRUE;
        }
    }
    endmntent(mounts);
#elif defined(HAVE_GETMNTINFO)
    entries = getmntinfo(&fsp, MNT_NOWAIT);
    if (entries < 0)
        return FALSE;

    while (entries-- > 0) {
        if (!strcmp(fsp->f_mntfromname, devname))
            return TRUE;
        fsp++;
    }
#endif
#endif
    return FALSE;
}


gboolean
cdda_get_toc(cdda_disc_toc_t * info, const char *device)
{
    gboolean retv = FALSE;
    int fd;

    if (is_mounted(device))
        return FALSE;

    if ((fd = open(device, CDOPENFLAGS)) == -1)
        return FALSE;

    memset(info, 0, sizeof(cdda_disc_toc_t));

    retv = cdda_get_toc_lowlevel(fd, info);
    close(fd);

    return retv;
}

static void
cdda_init(void)
{
    ConfigDb *db;
    struct driveinfo *drive = g_malloc0(sizeof(struct driveinfo));
    int ndrives = 1, i;

    cdda_playing.fd = -1;
    memset(&cdda_cfg, 0, sizeof(CDDAConfig));

#ifdef HAVE_OSS
    drive->mixer = CDDA_MIXER_OSS;
    drive->oss_mixer = SOUND_MIXER_CD;
#endif

    db = bmp_cfg_db_open();

    /* These names are used for backwards compatibility */
    bmp_cfg_db_get_string(db, "CDDA", "device", &drive->device);
    bmp_cfg_db_get_string(db, "CDDA", "directory", &drive->directory);
    bmp_cfg_db_get_int(db, "CDDA", "mixer", &drive->mixer);
    bmp_cfg_db_get_int(db, "CDDA", "readmode", &drive->dae);

    if (!drive->device)
        drive->device = g_strdup(CDDA_DEVICE);
    if (!drive->directory)
        drive->directory = g_strdup(CDDA_DIRECTORY);

    cdda_cfg.drives = g_list_append(cdda_cfg.drives, drive);

    bmp_cfg_db_get_int(db, "CDDA", "num_drives", &ndrives);
    for (i = 1; i < ndrives; i++) {
        char label[20];
        drive = g_malloc0(sizeof(struct driveinfo));

        sprintf(label, "device%d", i);
        bmp_cfg_db_get_string(db, "CDDA", label, &drive->device);

        sprintf(label, "directory%d", i);
        bmp_cfg_db_get_string(db, "CDDA", label, &drive->directory);

        sprintf(label, "mixer%d", i);
        bmp_cfg_db_get_int(db, "CDDA", label, &drive->mixer);

        sprintf(label, "readmode%d", i);
        bmp_cfg_db_get_int(db, "CDDA", label, &drive->dae);

        cdda_cfg.drives = g_list_append(cdda_cfg.drives, drive);
    }
    bmp_cfg_db_get_bool(db, "CDDA", "title_override",
                        &cdda_cfg.title_override);
    bmp_cfg_db_get_string(db, "CDDA", "name_format", &cdda_cfg.name_format);
    bmp_cfg_db_get_bool(db, "CDDA", "use_cddb", &cdda_cfg.use_cddb);
    bmp_cfg_db_get_string(db, "CDDA", "cddb_server", &cdda_cfg.cddb_server);
#ifdef WITH_CDINDEX
    bmp_cfg_db_get_bool(db, "CDDA", "use_cdin", &cdda_cfg.use_cdin);
#else
    cdda_cfg.use_cdin = FALSE;
#endif
    bmp_cfg_db_get_string(db, "CDDA", "cdin_server", &cdda_cfg.cdin_server);
    bmp_cfg_db_close(db);

    if (!cdda_cfg.cdin_server)
        cdda_cfg.cdin_server = g_strdup("www.cdindex.org");
    if (!cdda_cfg.cddb_server)
        cdda_cfg.cddb_server = g_strdup(CDDB_DEFAULT_SERVER);
    if (!cdda_cfg.name_format)
        cdda_cfg.name_format = g_strdup("%p - %t");
}

struct driveinfo *
cdda_find_drive(char *filename)
{
    GList *node;

    // FIXME: Will always return the first drive

    for (node = cdda_cfg.drives; node; node = node->next) {
        struct driveinfo *d = node->data;
        if (!strncmp(d->directory, filename, strlen(d->directory)))
            return d;
    }

    return NULL;

}

static void
timeout_destroy(struct timeout *entry)
{
    g_free(entry->device);
    g_free(entry);
    timeout_list = g_list_remove(timeout_list, entry);
}

static void
timeout_remove_for_device(char *device)
{
    GList *node;

    for (node = timeout_list; node; node = node->next) {
        struct timeout *t = node->data;

        if (!strcmp(t->device, device)) {
            gtk_timeout_remove(t->id);
            timeout_destroy(t);
            return;
        }
    }

}

static void
cleanup(void)
{
    GList *node;
    struct driveinfo *drive;

    g_free(cdda_ip.description);
    cdda_ip.description = NULL;

    if (cdda_cfg.drives) {
        for (node = g_list_first(cdda_cfg.drives); node; node = node->next) {
            drive = (struct driveinfo *)node->data;
            if (!drive)
                continue;

            if (drive->device)
                free(drive->device);

            if (drive->directory)
                free(drive->directory);

            free(drive);
        }

        g_list_free(cdda_cfg.drives);
        cdda_cfg.drives = NULL;
    }

    if (cdda_cfg.name_format) {
        free(cdda_cfg.name_format);
        cdda_cfg.name_format = NULL;
    }

    if (cdda_cfg.cddb_server) {
        free(cdda_cfg.cddb_server);
        cdda_cfg.cddb_server = NULL;
    }

    if (cdda_cfg.cdin_server) {
        free(cdda_cfg.cdin_server);
        cdda_cfg.cdin_server = NULL;
    }

    while (timeout_list) {
        struct timeout *t = timeout_list->data;
        gtk_timeout_remove(t->id);
        stop_timeout(t);
        timeout_destroy(t);
    }
    cddb_quit();
}

static int
is_our_file(char *filename)
{
    char *ext = ".cda";

    if (cdda_find_drive(filename) == NULL) {
        return FALSE;
    }

    if (g_str_has_suffix(filename, ext)) {
        return TRUE;
    }
    return FALSE;
}


static GList *
scan_dir(char *dir)
{
    GList *list = NULL;
    int i;
    cdda_disc_toc_t toc;
    struct driveinfo *drive;

    if ((drive = cdda_find_drive(dir)) == NULL)
        return NULL;

    if (!cdda_get_toc(&toc, drive->device))
        return NULL;

    for (i = toc.last_track; i >= toc.first_track; i--)
        if (!toc.track[i].flags.data_track) {
            list = g_list_prepend(list, g_strdup_printf("Track %02d.cda", i));
        }
    return list;
}

guint
cdda_calculate_track_length(cdda_disc_toc_t * toc, int track)
{
    if (track == toc->last_track)
        return (LBA(toc->leadout) - LBA(toc->track[track]));
    else
        return (LBA(toc->track[track + 1]) - LBA(toc->track[track]));
}

static void *
dae_play_loop(void *arg)
{
    char *buffer = g_malloc(CD_FRAMESIZE_RAW * CDDA_DAE_FRAMES);
    int pos = LBA(cdda_playing.cd_toc.track[cdda_playing.track]);
    int end, frames;

    if (cdda_playing.track == cdda_playing.cd_toc.last_track)
        end = LBA(cdda_playing.cd_toc.leadout);
    else
        end = LBA(cdda_playing.cd_toc.track[cdda_playing.track + 1]);

    while (cdda_playing.playing) {
        int left;
        char *data;

        if (dae_data.seek != -1) {
            cdda_ip.output->flush(dae_data.seek * 1000);
            pos = LBA(cdda_playing.cd_toc.track[cdda_playing.track])
                + dae_data.seek * 75;
            dae_data.seek = -1;
            dae_data.eof = FALSE;
        }
        frames = MIN(CDDA_DAE_FRAMES, end - pos);
        if (frames == 0)
            dae_data.eof = TRUE;

        if (dae_data.eof) {
            xmms_usleep(30000);
            continue;
        }

        frames = read_audio_data(cdda_playing.fd, pos, frames, buffer);
        if (frames <= 0) {
            int err = -frames;
            if (err == EOPNOTSUPP)
                dae_data.eof = TRUE;
            else {
                /*
                 * If the read failed, skip ahead to
                 * avoid getting stuck on scratches
                 * and such.
                 */
                g_message("read_audio_data() failed:  %s (%d)",
                          strerror(err), err);
                pos += MIN(CDDA_DAE_FRAMES, end - pos);
            }
            continue;
        }
        left = frames * CD_FRAMESIZE_RAW;
        data = buffer;
        while (cdda_playing.playing && left > 0 && dae_data.seek == -1) {
            int cur = MIN(512 * 2 * 2, left);
            cdda_ip.add_vis_pcm(cdda_ip.output->written_time(),
                                FMT_S16_LE, 2, cur, data);
            while (cdda_ip.output->buffer_free() < cur &&
                   cdda_playing.playing && dae_data.seek == -1)
                xmms_usleep(30000);
            if (cdda_playing.playing && dae_data.seek == -1)
                produce_audio(cdda_ip.output->written_time(), FMT_S16_LE, 2, cur, data, &cdda_playing.playing);
            left -= cur;
            data += cur;
        }
        pos += frames;
    }

    cdda_ip.output->buffer_free();
    cdda_ip.output->buffer_free();
    g_free(buffer);

    g_thread_exit(NULL);
    return NULL;
}

static void
dae_play(void)
{
    if (cdda_ip.output->open_audio(FMT_S16_LE, 44100, 2) == 0) {
        dae_data.audio_error = TRUE;
        cdda_playing.playing = FALSE;
        return;
    }
    dae_data.seek = -1;
    dae_data.eof = FALSE;
    dae_data.audio_error = FALSE;
    dae_data.thread = g_thread_create(dae_play_loop, NULL, TRUE, NULL);
}

static void
play_file(char *filename)
{
    char *tmp;
    struct driveinfo *drive;
    int track;
    int track_len;

//      g_message(g_strdup_printf("** CD_AUDIO: trying to play file %s",filename));

    if ((drive = cdda_find_drive(filename)) == NULL) {
//              g_message("** CD_AUDIO: find drive check failed");
        return;
    }
    if (is_mounted(drive->device)) {
//              g_message("** CD_AUDIO: drive is mounted");
        return;
    }
    tmp = strrchr(filename, '/');
    if (tmp)
        tmp++;
    else
        tmp = filename;

    if (!sscanf(tmp, "Track %d.cda", &track)) {
//              g_message("** CD_AUDIO: filename check failed");                
        return;
    }

    if (!cdda_get_toc(&cdda_playing.cd_toc, drive->device) ||
        cdda_playing.cd_toc.track[track].flags.data_track ||
        track < cdda_playing.cd_toc.first_track ||
        track > cdda_playing.cd_toc.last_track) {
//              g_message("** CD_AUDIO: toc check failed");             
        return;
    }

    if ((cdda_playing.fd = open(drive->device, CDOPENFLAGS)) == -1) {
//              g_message("** CD_AUDIO: device open failed");           
        return;
    }
    track_len = cdda_calculate_track_length(&cdda_playing.cd_toc, track);
    cdda_ip.set_info(get_song_title(cdda_get_tuple(&cdda_playing.cd_toc, track)),
                     (track_len * 1000) / 75, 44100 * 2 * 2 * 8, 44100, 2);

    memcpy(&cdda_playing.drive, drive, sizeof(struct driveinfo));
#ifndef CDDA_HAS_READAUDIO
    cdda_playing.drive.dae = FALSE;
#endif

    cdda_playing.track = track;

    is_paused = FALSE;
    timeout_remove_for_device(drive->device);

    cdda_playing.playing = TRUE;
    if (drive->dae)
        dae_play();
    else
        seek(0);
}

static TitleInput *
cdda_get_tuple(cdda_disc_toc_t * toc, int track)
{
    G_LOCK_DEFINE_STATIC(tuple);

    static guint32 cached_id;
    static cdinfo_t cdinfo;
    static gchar *performer, *album_name, *track_name;
    TitleInput *tuple = NULL;
    guint32 disc_id;

    disc_id = cdda_cddb_compute_discid(toc);

    /*
     * We want to avoid looking up a album from two threads simultaneously.
     * This can happen since we are called both from the main-thread and
     * from the playlist-thread.
     */

    G_LOCK(tuple);
    if (!(disc_id == cached_id && cdinfo.is_valid)) {
        /*
         * We try to look up the disc again if the info is not
         * valid.  The user might have configured a new server
         * in the meantime.
         */
        cdda_cdinfo_flush(&cdinfo);
        cached_id = disc_id;

        if (!cdda_cdinfo_read_file(disc_id, &cdinfo)) {
            if (cdda_cfg.use_cddb)
                cdda_cddb_get_info(toc, &cdinfo);
            if (cdinfo.is_valid)
                cdda_cdinfo_write_file(disc_id, &cdinfo);
        }
    }

    tuple = bmp_title_input_new();
    cdda_cdinfo_get(&cdinfo, track, &performer, &album_name, &track_name);
    G_UNLOCK(tuple);

    tuple->performer = g_strdup(performer);
    tuple->album_name = g_strdup(album_name);
    tuple->track_name = g_strdup(track_name);
    tuple->track_number = (track);
    tuple->file_name = g_strdup(tuple->file_path);
    tuple->file_path = g_strdup_printf(_("CD Audio Track %02u"), track);
    tuple->file_ext = "cda";
    tuple->length = ((cdda_calculate_track_length(toc, track) * 1000) / 75);

    if (!tuple->track_name)
        tuple->track_name = g_strdup_printf(_("CD Audio Track %02u"), track);

    return tuple;
}

static gchar *
get_song_title(TitleInput *tuple)
{
    return xmms_get_titlestring(cdda_cfg.title_override ?
                                cdda_cfg.name_format :
                                xmms_get_gentitle_format(), tuple);
}

static gboolean
stop_timeout(gpointer data)
{
    int fd;
    struct timeout *to = data;

    fd = open(to->device, CDOPENFLAGS);
    if (fd != -1) {
        ioctl(fd, XMMS_STOP, 0);
        close(fd);
    }
    timeout_destroy(to);
    return FALSE;
}

static void
stop(void)
{
    struct timeout *to_info;
    if (cdda_playing.fd < 0)
        return;

    cdda_playing.playing = FALSE;

    if (cdda_playing.drive.dae) {
        g_thread_join(dae_data.thread);
        cdda_ip.output->close_audio();
    }
    else
        ioctl(cdda_playing.fd, XMMS_PAUSE, 0);

    close(cdda_playing.fd);
    cdda_playing.fd = -1;

    if (!cdda_playing.drive.dae) {
        to_info = g_malloc(sizeof(*to_info));
        to_info->device = g_strdup(cdda_playing.drive.device);
        to_info->id = gtk_timeout_add(STOP_DELAY * 100, stop_timeout,
                                      to_info);
        timeout_list = g_list_prepend(timeout_list, to_info);
    }
}

static void
cdda_pause(short p)
{
    if (cdda_playing.drive.dae) {
        cdda_ip.output->pause(p);
        return;
    }
    if (p) {
        pause_time = get_time();
        ioctl(cdda_playing.fd, XMMS_PAUSE, 0);
    }
    else {
        ioctl(cdda_playing.fd, XMMS_RESUME, 0);
        pause_time = -1;
    }
    is_paused = p;
}



static void
seek(int time)
{
    struct cdda_msf *end, start;
    int track = cdda_playing.track;

//      g_message("** CD_AUDIO: seeking...");
    if (cdda_playing.drive.dae) {
        dae_data.seek = time;
        while (dae_data.seek != -1)
            xmms_usleep(20000);
        return;
    }

    start.minute = (cdda_playing.cd_toc.track[track].minute * 60 +
                    cdda_playing.cd_toc.track[track].second + time) / 60;
    start.second = (cdda_playing.cd_toc.track[track].second + time) % 60;
    start.frame = cdda_playing.cd_toc.track[track].frame;
    if (track == cdda_playing.cd_toc.last_track)
        end = &cdda_playing.cd_toc.leadout;
    else
        end = &cdda_playing.cd_toc.track[track + 1];

    play_ioctl(&start, end);

    if (is_paused) {
        cdda_pause(TRUE);
        pause_time = time * 1000;
    }
}

static int
get_time_analog(void)
{
    int frame, start_frame, length;
    int track = cdda_playing.track;

    if (is_paused && pause_time != -1)
        return pause_time;

    frame = get_current_frame();

    if (frame == -1)
        return -1;

    start_frame = LBA(cdda_playing.cd_toc.track[track]);
    length = cdda_calculate_track_length(&cdda_playing.cd_toc, track);

    if (frame - start_frame >= length - 20) /* 20 seems to work better */
        return -1;

    return ((frame - start_frame) * 1000) / 75;
}

static int
get_time_dae(void)
{
    if (dae_data.audio_error)
        return -2;
    if (!cdda_playing.playing ||
        (dae_data.eof && !cdda_ip.output->buffer_playing()))
        return -1;
    return cdda_ip.output->output_time();
}

static int
get_time(void)
{
    if (cdda_playing.fd == -1)
        return -1;

    if (cdda_playing.drive.dae)
        return get_time_dae();
    else
        return get_time_analog();
}

static void
get_song_info(char *filename, char **title, int *len)
{
    cdda_disc_toc_t toc;
    int t;
    char *tmp;
    struct driveinfo *drive;
    TitleInput *tuple;

    *title = NULL;
    *len = -1;

//      g_message("** CD_AUDIO: getting song info");

    if ((drive = cdda_find_drive(filename)) == NULL)
        return;

    tmp = strrchr(filename, '/');
    if (tmp)
        tmp++;
    else
        tmp = filename;

    if (!sscanf(tmp, "Track %d.cda", &t))
        return;
    if (!cdda_get_toc(&toc, drive->device))
        return;
    if (t < toc.first_track || t > toc.last_track
        || toc.track[t].flags.data_track)
        return;

    if ((tuple = cdda_get_tuple(&toc, t)) != NULL) {
        *len = tuple->length;
        *title = get_song_title(tuple);
    }
    bmp_title_input_free(tuple);
}

static TitleInput *
get_song_tuple(char *filename)
{
    cdda_disc_toc_t toc;
    int t;
    char *tmp;
    struct driveinfo *drive;
    TitleInput *tuple = NULL;

//      g_message("** CD_AUDIO: getting song info");

    if ((drive = cdda_find_drive(filename)) == NULL)
        return tuple;

    tmp = strrchr(filename, '/');
    if (tmp)
        tmp++;
    else
        tmp = filename;

    if (!sscanf(tmp, "Track %d.cda", &t))
        return tuple;
    if (!cdda_get_toc(&toc, drive->device))
        return tuple;
    if (t < toc.first_track || t > toc.last_track
        || toc.track[t].flags.data_track)
        return tuple;

    tuple = cdda_get_tuple(&toc, t);
    return tuple;
}

#ifdef HAVE_OSS
static void
oss_get_volume(int *l, int *r, int mixer_line)
{
    int fd, v;

    fd = open(DEV_MIXER, O_RDONLY);
    if (fd != -1) {
        ioctl(fd, MIXER_READ(mixer_line), &v);
        *r = (v & 0xFF00) >> 8;
        *l = (v & 0x00FF);
        close(fd);
    }
}

static void
oss_set_volume(int l, int r, int mixer_line)
{
    int fd, v;

    fd = open(DEV_MIXER, O_RDONLY);
    if (fd != -1) {
        v = (r << 8) | l;
        ioctl(fd, MIXER_WRITE(mixer_line), &v);
        close(fd);
    }
}
#else
static void
oss_get_volume(int *l, int *r, int mixer_line)
{
}
static void
oss_set_volume(int l, int r, int mixer_line)
{
}
#endif


static void
get_volume(int *l, int *r)
{
    if (cdda_playing.drive.dae)
        cdda_ip.output->get_volume(l, r);
    else if (cdda_playing.drive.mixer == CDDA_MIXER_OSS)
        oss_get_volume(l, r, cdda_playing.drive.oss_mixer);
    else if (cdda_playing.drive.mixer == CDDA_MIXER_DRIVE)
        drive_get_volume(l, r);
}

static void
set_volume(int l, int r)
{
    if (cdda_playing.drive.dae)
        cdda_ip.output->set_volume(l, r);
    else if (cdda_playing.drive.mixer == CDDA_MIXER_OSS)
        oss_set_volume(l, r, cdda_playing.drive.oss_mixer);
    else if (cdda_playing.drive.mixer == CDDA_MIXER_DRIVE)
        drive_set_volume(l, r);
}
