#include "mpg123.h"
#include "common.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "libaudacious/util.h"
#include "libaudacious/configdb.h"
#include "libaudacious/vfs.h"
#include "libaudacious/titlestring.h"
#include "audacious/util.h"
#include <tag_c.h>

#define BUFSIZE_X	2048

static struct frame fr, temp_fr;

PlayerInfo *mpgdec_info = NULL;
static GThread *decode_thread;

static gboolean audio_error = FALSE, output_opened = FALSE, dopause = FALSE;
gint mpgdec_bitrate, mpgdec_frequency, mpgdec_length, mpgdec_layer,
    mpgdec_lsf;
gchar *mpgdec_title = NULL, *mpgdec_filename = NULL;
static int disp_bitrate, skip_frames = 0;
static int cpu_fflags, cpu_efflags;
gboolean mpgdec_stereo, mpgdec_mpeg25;
int mpgdec_mode;

mpgdec_t *ins;

gchar **mpgdec_id3_encoding_list = NULL;

static TagLib_File *taglib_file;
static TagLib_Tag *taglib_tag;
static const TagLib_AudioProperties *taglib_ap;

const char *mpgdec_id3_genres[GENRE_MAX] = {
    N_("Blues"), N_("Classic Rock"), N_("Country"), N_("Dance"),
    N_("Disco"), N_("Funk"), N_("Grunge"), N_("Hip-Hop"),
    N_("Jazz"), N_("Metal"), N_("New Age"), N_("Oldies"),
    N_("Other"), N_("Pop"), N_("R&B"), N_("Rap"), N_("Reggae"),
    N_("Rock"), N_("Techno"), N_("Industrial"), N_("Alternative"),
    N_("Ska"), N_("Death Metal"), N_("Pranks"), N_("Soundtrack"),
    N_("Euro-Techno"), N_("Ambient"), N_("Trip-Hop"), N_("Vocal"),
    N_("Jazz+Funk"), N_("Fusion"), N_("Trance"), N_("Classical"),
    N_("Instrumental"), N_("Acid"), N_("House"), N_("Game"),
    N_("Sound Clip"), N_("Gospel"), N_("Noise"), N_("AlternRock"),
    N_("Bass"), N_("Soul"), N_("Punk"), N_("Space"),
    N_("Meditative"), N_("Instrumental Pop"),
    N_("Instrumental Rock"), N_("Ethnic"), N_("Gothic"),
    N_("Darkwave"), N_("Techno-Industrial"), N_("Electronic"),
    N_("Pop-Folk"), N_("Eurodance"), N_("Dream"),
    N_("Southern Rock"), N_("Comedy"), N_("Cult"),
    N_("Gangsta Rap"), N_("Top 40"), N_("Christian Rap"),
    N_("Pop/Funk"), N_("Jungle"), N_("Native American"),
    N_("Cabaret"), N_("New Wave"), N_("Psychedelic"), N_("Rave"),
    N_("Showtunes"), N_("Trailer"), N_("Lo-Fi"), N_("Tribal"),
    N_("Acid Punk"), N_("Acid Jazz"), N_("Polka"), N_("Retro"),
    N_("Musical"), N_("Rock & Roll"), N_("Hard Rock"), N_("Folk"),
    N_("Folk/Rock"), N_("National Folk"), N_("Swing"),
    N_("Fast-Fusion"), N_("Bebob"), N_("Latin"), N_("Revival"),
    N_("Celtic"), N_("Bluegrass"), N_("Avantgarde"),
    N_("Gothic Rock"), N_("Progressive Rock"),
    N_("Psychedelic Rock"), N_("Symphonic Rock"), N_("Slow Rock"),
    N_("Big Band"), N_("Chorus"), N_("Easy Listening"),
    N_("Acoustic"), N_("Humour"), N_("Speech"), N_("Chanson"),
    N_("Opera"), N_("Chamber Music"), N_("Sonata"), N_("Symphony"),
    N_("Booty Bass"), N_("Primus"), N_("Porn Groove"),
    N_("Satire"), N_("Slow Jam"), N_("Club"), N_("Tango"),
    N_("Samba"), N_("Folklore"), N_("Ballad"), N_("Power Ballad"),
    N_("Rhythmic Soul"), N_("Freestyle"), N_("Duet"),
    N_("Punk Rock"), N_("Drum Solo"), N_("A Cappella"),
    N_("Euro-House"), N_("Dance Hall"), N_("Goa"),
    N_("Drum & Bass"), N_("Club-House"), N_("Hardcore"),
    N_("Terror"), N_("Indie"), N_("BritPop"), N_("Negerpunk"),
    N_("Polsk Punk"), N_("Beat"), N_("Christian Gangsta Rap"),
    N_("Heavy Metal"), N_("Black Metal"), N_("Crossover"),
    N_("Contemporary Christian"), N_("Christian Rock"),
    N_("Merengue"), N_("Salsa"), N_("Thrash Metal"),
    N_("Anime"), N_("JPop"), N_("Synthpop")
};

double
mpgdec_compute_tpf(struct frame *fr)
{
    const int bs[4] = { 0, 384, 1152, 1152 };
    double tpf;

    tpf = bs[fr->lay];
    tpf /= mpgdec_freqs[fr->sampling_frequency] << (fr->lsf);
    return tpf;
}

static void
set_synth_functions(struct frame *fr)
{
    typedef int (*func) (mpgdec_real *, int, unsigned char *, int *);
    typedef int (*func_mono) (mpgdec_real *, unsigned char *, int *);
    typedef void (*func_dct36) (mpgdec_real *, mpgdec_real *, mpgdec_real *, mpgdec_real *, mpgdec_real *);

    int ds = fr->down_sample;
    int p8 = 0;

    static func funcs[2][2] = {
        {mpgdec_synth_1to1,
         mpgdec_synth_ntom},
        {mpgdec_synth_1to1_8bit,
	 mpgdec_synth_ntom_8bit}
    };

    static func_mono funcs_mono[2][2] = {
        {mpgdec_synth_1to1_mono,
         mpgdec_synth_ntom_mono},
        {mpgdec_synth_1to1_8bit_mono,
         mpgdec_synth_ntom_8bit_mono}
    };

    /* Compatibility with older configs. */
    if (ds > 1)
	ds = 1;

    if (mpgdec_cfg.resolution == 8)
        p8 = 1;
    fr->synth = funcs[p8][ds];
    fr->synth_mono = funcs_mono[p8][ds];
    fr->synth_type = SYNTH_FPU;

    if (p8) {
        mpgdec_make_conv16to8_table();
    }
}

static void
init(void)
{
    ConfigDb *db;
    gchar *tmp = NULL;

    ins = mpgdec_new();

    memset(&mpgdec_cfg, 0, sizeof(mpgdec_cfg));

    mpgdec_cfg.resolution = 16;
    mpgdec_cfg.channels = 2;
    mpgdec_cfg.downsample = 0;
    mpgdec_cfg.http_buffer_size = 128;
    mpgdec_cfg.http_prebuffer = 25;
    mpgdec_cfg.proxy_port = 8080;
    mpgdec_cfg.proxy_use_auth = FALSE;
    mpgdec_cfg.proxy_user = NULL;
    mpgdec_cfg.proxy_pass = NULL;
    mpgdec_cfg.use_udp_channel = TRUE;
    mpgdec_cfg.title_override = FALSE;
    mpgdec_cfg.disable_id3v2 = FALSE;
    mpgdec_cfg.default_synth = SYNTH_AUTO;

    mpgdec_cfg.title_encoding_enabled = FALSE;
    mpgdec_cfg.title_encoding = NULL;

    db = bmp_cfg_db_open();

    bmp_cfg_db_get_int(db, "MPG123", "resolution", &mpgdec_cfg.resolution);
    bmp_cfg_db_get_int(db, "MPG123", "channels", &mpgdec_cfg.channels);
    bmp_cfg_db_get_int(db, "MPG123", "downsample", &mpgdec_cfg.downsample);
    bmp_cfg_db_get_int(db, "MPG123", "http_buffer_size",
                       &mpgdec_cfg.http_buffer_size);
    bmp_cfg_db_get_int(db, "MPG123", "http_prebuffer",
                       &mpgdec_cfg.http_prebuffer);
    bmp_cfg_db_get_bool(db, "MPG123", "save_http_stream",
                        &mpgdec_cfg.save_http_stream);
    if (!bmp_cfg_db_get_string
        (db, "MPG123", "save_http_path", &mpgdec_cfg.save_http_path))
        mpgdec_cfg.save_http_path = g_strdup(g_get_home_dir());

    bmp_cfg_db_get_bool(db, "MPG123", "use_udp_channel",
                        &mpgdec_cfg.use_udp_channel);

    bmp_cfg_db_get_bool(db, "MPG123", "title_override",
                        &mpgdec_cfg.title_override);
    bmp_cfg_db_get_bool(db, "MPG123", "disable_id3v2",
                        &mpgdec_cfg.disable_id3v2);
    if (!bmp_cfg_db_get_string
        (db, "MPG123", "id3_format", &mpgdec_cfg.id3_format))
        mpgdec_cfg.id3_format = g_strdup("%p - %t");
    bmp_cfg_db_get_int(db, "MPG123", "default_synth",
                       &mpgdec_cfg.default_synth);

    bmp_cfg_db_get_bool(db, "MPG123", "title_encoding_enabled", &mpgdec_cfg.title_encoding_enabled);
    bmp_cfg_db_get_string(db, "MPG123", "title_encoding", &mpgdec_cfg.title_encoding);
    if (mpgdec_cfg.title_encoding_enabled)
        mpgdec_id3_encoding_list = g_strsplit_set(mpgdec_cfg.title_encoding, ENCODING_SEPARATOR, 0);

    bmp_cfg_db_get_bool(db, NULL, "use_proxy", &mpgdec_cfg.use_proxy);
    bmp_cfg_db_get_string(db, NULL, "proxy_host", &mpgdec_cfg.proxy_host);
    bmp_cfg_db_get_string(db, NULL, "proxy_port", &tmp);

    if (tmp != NULL)
	mpgdec_cfg.proxy_port = atoi(tmp);

    bmp_cfg_db_get_bool(db, NULL, "proxy_use_auth", &mpgdec_cfg.proxy_use_auth);
    bmp_cfg_db_get_string(db, NULL, "proxy_user", &mpgdec_cfg.proxy_user);
    bmp_cfg_db_get_string(db, NULL, "proxy_pass", &mpgdec_cfg.proxy_pass);

    bmp_cfg_db_close(db);

    if (mpgdec_cfg.resolution != 16 && mpgdec_cfg.resolution != 8)
        mpgdec_cfg.resolution = 16;

    mpgdec_cfg.channels = CLAMP(mpgdec_cfg.channels, 0, 2);
    mpgdec_cfg.downsample = CLAMP(mpgdec_cfg.downsample, 0, 2);
    mpgdec_getcpuflags(&cpu_fflags, &cpu_efflags);
}

static void
cleanup(void)
{
    g_free(mpgdec_ip.description);
    mpgdec_ip.description = NULL;

    if (mpgdec_cfg.save_http_path) {
        free(mpgdec_cfg.save_http_path);
        mpgdec_cfg.save_http_path = NULL;
    }

    if (mpgdec_cfg.proxy_host) {
        free(mpgdec_cfg.proxy_host);
        mpgdec_cfg.proxy_host = NULL;
    }

    if (mpgdec_cfg.proxy_user) {
        free(mpgdec_cfg.proxy_user);
        mpgdec_cfg.proxy_user = NULL;
    }

    if (mpgdec_cfg.proxy_pass) {
        free(mpgdec_cfg.proxy_pass);
        mpgdec_cfg.proxy_pass = NULL;
    }

    if (mpgdec_cfg.id3_format) {
        free(mpgdec_cfg.id3_format);
        mpgdec_cfg.id3_format = NULL;
    }

    if (mpgdec_cfg.title_encoding) {
        free(mpgdec_cfg.title_encoding);
        mpgdec_cfg.title_encoding = NULL;
    }

    g_strfreev(mpgdec_id3_encoding_list);
}

static guint32
convert_to_header(guint8 * buf)
{
    return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}


#if 0
#define DET_BUF_SIZE 1024

static gboolean
mpgdec_detect_by_content(char *filename)
{
    VFSFile *file;
    guchar tmp[4];
    guint32 head;
    struct frame fr;
    guchar buf[DET_BUF_SIZE];
    int in_buf, i;
    gboolean ret = FALSE;
    guint cyc = 0;

    if ((file = vfs_fopen(filename, "rb")) == NULL)
        return FALSE;
    if (vfs_fread(tmp, 1, 4, file) != 4)
        goto done;
    head = convert_to_header(tmp);
    while (!mpgdec_head_check(head)) {
        /*
         * The mpeg-stream can start anywhere in the file,
         * so we check the entire file
	 *
	 * Incorrect! We give up past ten iterations of this
	 * code for safety's sake. Buffer overflows suck. --nenolod
         */
        /* Optimize this */
        in_buf = vfs_fread(buf, 1, DET_BUF_SIZE, file);
        if (in_buf == 0)
            goto done;

        for (i = 0; i < in_buf; i++) {
            head <<= 8;
            head |= buf[i];
            if (mpgdec_head_check(head)) {
                vfs_fseek(file, i + 1 - in_buf, SEEK_CUR);
                break;
            }
        }

        if (++cyc > 20)
	    goto done;
    }
    if (mpgdec_decode_header(&fr, head)) {
        /*
         * We found something which looks like a MPEG-header.
         * We check the next frame too, to be sure
         */

        if (vfs_fseek(file, fr.framesize, SEEK_CUR) != 0)
            goto done;
        if (vfs_fread(tmp, 1, 4, file) != 4)
            goto done;
        head = convert_to_header(tmp);
        if (mpgdec_head_check(head) && mpgdec_decode_header(&fr, head))
            ret = TRUE;
    }

  done:
    vfs_fclose(file);
    return ret;
}
#endif

static int
is_our_file(char *filename)
{
    gchar *ext = strrchr(filename, '.');

    if (!strncasecmp(filename, "http://", 7) && (ext && strncasecmp(ext, ".ogg", 4)))
	return TRUE;
    else if (ext && (!strncasecmp(ext, ".mp3", 4)
	|| !strncasecmp(ext, ".mp2", 4)
	|| !strncasecmp(ext, ".mpg", 4)))
        return TRUE;

    return FALSE;
}

static void
play_frame(struct frame *fr)
{
    if (fr->error_protection) {
        bsi.wordpointer += 2;
        /*  mpgdec_getbits(16); *//* skip crc */
    }
    if (!fr->do_layer(fr)) {
        skip_frames = 2;
        mpgdec_info->output_audio = FALSE;
    }
    else {
        if (!skip_frames)
            mpgdec_info->output_audio = TRUE;
        else
            skip_frames--;
    }
}

static const char *
get_id3_genre(unsigned char genre_code)
{
    if (genre_code < GENRE_MAX)
        return gettext(mpgdec_id3_genres[genre_code]);

    return "";
}

guint
mpgdec_strip_spaces(char *src, size_t n)
{
    gchar *space = NULL,        /* last space in src */
        *start = src;

    while (n--)
        switch (*src++) {
        case '\0':
            n = 0;              /* breaks out of while loop */

            src--;
            break;
        case ' ':
            if (space == NULL)
                space = src - 1;
            break;
        default:
            space = NULL;       /* don't terminate intermediate spaces */

            break;
        }
    if (space != NULL) {
        src = space;
        *src = '\0';
    }
    return src - start;
}

/*
 * Function extname (filename)
 *
 *    Return pointer within filename to its extenstion, or NULL if
 *    filename has no extension.
 *
 */
static gchar *
extname(const char *filename)
{
    gchar *ext = strrchr(filename, '.');

    if (ext != NULL)
        ++ext;

    return ext;
}

/*
 * Function id3v1_to_id3v2 (v1, v2)
 *
 *    Convert ID3v1 tag `v1' to ID3v2 tag `v2'.
 *
 */
void
mpgdec_id3v1_to_id3v2(struct id3v1tag_t *v1, struct id3tag_t *v2)
{
    memset(v2, 0, sizeof(struct id3tag_t));
    strncpy(v2->title, v1->title, 30);
    strncpy(v2->artist, v1->artist, 30);
    strncpy(v2->album, v1->album, 30);
    strncpy(v2->comment, v1->u.v1_0.comment, 30);
    strncpy(v2->genre, get_id3_genre(v1->genre), sizeof(v2->genre));
    g_strstrip(v2->title);
    g_strstrip(v2->artist);
    g_strstrip(v2->album);
    g_strstrip(v2->comment);
    g_strstrip(v2->genre);
    {
      char y[5];
      memcpy(y, v1->year, 4); y[4]=0;
      v2->year = atoi(y);
    }

    /* Check for v1.1 tags. */
    if (v1->u.v1_1.__zero == 0)
        v2->track_number = v1->u.v1_1.track_number;
    else
        v2->track_number = 0;
}

#define REMOVE_NONEXISTANT_TAG(x)   if (!*x) { x = NULL; }

static long
get_song_length(VFSFile * file)
{
    int len;
    char tmp[4];

    vfs_fseek(file, 0, SEEK_END);
    len = vfs_ftell(file);
    vfs_fseek(file, -128, SEEK_END);
    vfs_fread(tmp, 1, 3, file);
    if (!strncmp(tmp, "TAG", 3))
        len -= 128;
    return len;
}


static guint
get_song_time(VFSFile * file)
{
    guint32 head;
    guchar tmp[4], *buf;
    struct frame frm;
    xing_header_t xing_header;
    double tpf, bpf;
    guint32 len;

    if (!file)
        return -1;

    vfs_fseek(file, 0, SEEK_SET);
    if (vfs_fread(tmp, 1, 4, file) != 4)
        return 0;
    head = convert_to_header(tmp);
    while (!mpgdec_head_check(head)) {
        head <<= 8;
        if (vfs_fread(tmp, 1, 1, file) != 1)
            return 0;
        head |= tmp[0];
    }
    if (mpgdec_decode_header(&frm, head)) {
        buf = g_malloc(frm.framesize + 4);
        vfs_fseek(file, -4, SEEK_CUR);
        vfs_fread(buf, 1, frm.framesize + 4, file);
        tpf = mpgdec_compute_tpf(&frm);
        if (mpgdec_get_xing_header(&xing_header, buf)) {
            g_free(buf);
            if (xing_header.bytes == 0)
                xing_header.bytes = get_song_length(file);
            return (tpf * xing_header.frames * 1000);
        }
        g_free(buf);
        bpf = mpgdec_compute_bpf(&frm);
        len = get_song_length(file);
        return ((guint) (len / bpf) * tpf * 1000);
    }
    return 0;
}

static TitleInput *
get_song_tuple(char *filename)
{
    VFSFile *file;
    TitleInput *tuple = NULL;

#ifdef USE_CHARDET
    taglib_set_strings_unicode(FALSE);
#endif

    if ((file = vfs_fopen(filename, "rb")) != NULL)
    {
        tuple = bmp_title_input_new();

        taglib_file = taglib_file_new(filename);
        taglib_tag = NULL;

        if (taglib_file != NULL)
        {
            taglib_tag = taglib_file_tag(taglib_file);
            taglib_ap = taglib_file_audioproperties(taglib_file); /* XXX: chainsaw, what is this for? -nenolod */
        }

	if (taglib_tag != NULL)
	{
	    tuple->performer = g_strdup(taglib_tag_artist(taglib_tag));
            tuple->album_name = g_strdup(taglib_tag_album(taglib_tag));
            tuple->track_name = g_strdup(taglib_tag_title(taglib_tag));

            mpgdec_strip_spaces(tuple->performer, strlen(tuple->performer));
            mpgdec_strip_spaces(tuple->album_name, strlen(tuple->album_name));
            mpgdec_strip_spaces(tuple->track_name, strlen(tuple->track_name));

            tuple->year = taglib_tag_year(taglib_tag);
            tuple->track_number = taglib_tag_track(taglib_tag);
            tuple->genre = g_strdup(taglib_tag_genre(taglib_tag));
            tuple->comment = g_strdup(taglib_tag_comment(taglib_tag));

            /* remove any blank tags, fucking taglib */
            REMOVE_NONEXISTANT_TAG(tuple->performer);
            REMOVE_NONEXISTANT_TAG(tuple->album_name);
            REMOVE_NONEXISTANT_TAG(tuple->track_name);
            REMOVE_NONEXISTANT_TAG(tuple->genre);
            REMOVE_NONEXISTANT_TAG(tuple->comment);
        }

	if (tuple->performer != NULL)
	    tuple->performer = str_to_utf8(tuple->performer);

	if (tuple->album_name != NULL)
	    tuple->album_name = str_to_utf8(tuple->album_name);

	if (tuple->track_name != NULL)
	    tuple->track_name = str_to_utf8(tuple->track_name);

	if (tuple->comment != NULL)
	    tuple->comment = str_to_utf8(tuple->comment);

	tuple->file_name = g_path_get_basename(filename);
	tuple->file_path = g_path_get_dirname(filename);
	tuple->file_ext = extname(filename);
        tuple->length = get_song_time(file);

        taglib_file_free(taglib_file);
        taglib_tag_free_strings();
        vfs_fclose(file);
    }

    return tuple;
}

static gchar *
get_song_title(TitleInput *tuple)
{
    return xmms_get_titlestring(mpgdec_cfg.title_override ?
                                mpgdec_cfg.id3_format :
                                xmms_get_gentitle_format(), tuple);
}

static void
get_song_info(char *filename, char **title_real, int *len_real)
{
    TitleInput *tuple;

    (*len_real) = -1;
    (*title_real) = NULL;

    /*
     * TODO: Getting song info from http streams.
     */
    if (!strncasecmp(filename, "http://", 7))
        return;

    if ((tuple = get_song_tuple(filename)) != NULL) {
        (*len_real) = tuple->length;
        (*title_real) = get_song_title(tuple);
    }

    bmp_title_input_free(tuple);
}

static int
open_output(void)
{
    int r;
    AFormat fmt = mpgdec_cfg.resolution == 16 ? FMT_S16_NE : FMT_U8;
/*    int freq = mpgdec_freqs[fr.sampling_frequency] >> mpgdec_cfg.downsample; */
    int freq = mpgdec_frequency;
    int channels = mpgdec_cfg.channels == 2 ? fr.stereo : 1;
    r = mpgdec_ip.output->open_audio(fmt, freq, channels);

    if (r && dopause) {
        mpgdec_ip.output->pause(TRUE);
        dopause = FALSE;
    }

    return r;
}


static int
mpgdec_seek(struct frame *fr, xing_header_t * xh, gboolean vbr, int time)
{
    int jumped = -1;

    if (xh) {
        int percent = ((double) time * 100.0) /
            (mpgdec_info->num_frames * mpgdec_info->tpf);
        int byte = mpgdec_seek_point(xh, percent);
        jumped = mpgdec_stream_jump_to_byte(fr, byte);
    }
    else if (vbr && mpgdec_length > 0) {
        int byte = ((guint64) time * 1000 * mpgdec_info->filesize) /
            mpgdec_length;
        jumped = mpgdec_stream_jump_to_byte(fr, byte);
    }
    else {
        int frame = time / mpgdec_info->tpf;
        jumped = mpgdec_stream_jump_to_frame(fr, frame);
    }

    return jumped;
}


static void *
decode_loop(void *arg)
{
    gboolean have_xing_header = FALSE, vbr = FALSE;
    int disp_count = 0;
    char *filename = arg;
    xing_header_t xing_header;

    /* This is used by fileinfo on http streams */
    mpgdec_bitrate = 0;

    mpgdec_pcm_sample = g_malloc0(32768);
    mpgdec_pcm_point = 0;
    mpgdec_filename = filename;

    mpgdec_read_frame_init();

    mpgdec_open_stream(filename, -1);

    if (mpgdec_info->eof || !mpgdec_read_frame(&fr))
        mpgdec_info->eof = TRUE;

    if (!mpgdec_info->eof && mpgdec_info->going) {
        if (mpgdec_cfg.channels == 2)
            fr.single = -1;
        else
            fr.single = 3;

        fr.down_sample = mpgdec_cfg.downsample;
        fr.down_sample_sblimit = SBLIMIT >> mpgdec_cfg.downsample;
        set_synth_functions(&fr);

        mpgdec_info->tpf = mpgdec_compute_tpf(&fr);
        if (strncasecmp(filename, "http://", 7)) {
            if (mpgdec_stream_check_for_xing_header(&fr, &xing_header)) {
                mpgdec_info->num_frames = xing_header.frames;
                have_xing_header = TRUE;
                mpgdec_read_frame(&fr);
            }
        }

        for (;;) {
            memcpy(&temp_fr, &fr, sizeof(struct frame));
            if (!mpgdec_read_frame(&temp_fr)) {
                mpgdec_info->eof = TRUE;
                break;
            }
            if (fr.lay != temp_fr.lay ||
                fr.sampling_frequency != temp_fr.sampling_frequency ||
                fr.stereo != temp_fr.stereo || fr.lsf != temp_fr.lsf)
                memcpy(&fr, &temp_fr, sizeof(struct frame));
            else
                break;
        }

        if (!have_xing_header && strncasecmp(filename, "http://", 7))
            mpgdec_info->num_frames = mpgdec_calc_numframes(&fr);

        memcpy(&fr, &temp_fr, sizeof(struct frame));
        mpgdec_bitrate = tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];
        disp_bitrate = mpgdec_bitrate;
        mpgdec_frequency = mpgdec_freqs[fr.sampling_frequency];
        mpgdec_stereo = fr.stereo;
        mpgdec_layer = fr.lay;
        mpgdec_lsf = fr.lsf;
        mpgdec_mpeg25 = fr.mpeg25;
        mpgdec_mode = fr.mode;

	if (fr.down_sample)
	{
	    long n = mpgdec_freqs[fr.sampling_frequency];
	    long m = n / fr.down_sample;

	    mpgdec_synth_ntom_set_step(n, m);

	    mpgdec_frequency = (gint) m;
	}

        if (strncasecmp(filename, "http://", 7)) {
            mpgdec_length = mpgdec_info->num_frames * mpgdec_info->tpf * 1000;
            if (!mpgdec_title)
                mpgdec_title = get_song_title(get_song_tuple(filename));
        }
        else {
            if (!mpgdec_title)
                mpgdec_title = mpgdec_http_get_title(filename);
            mpgdec_length = -1;
        }

        set_synth_functions(&fr);
        mpgdec_init_layer3(fr.down_sample_sblimit);

        mpgdec_ip.set_info(mpgdec_title, mpgdec_length,
                           mpgdec_bitrate * 1000,
                           mpgdec_freqs[fr.sampling_frequency], fr.stereo);

        output_opened = TRUE;

        if (!open_output()) {
            audio_error = TRUE;
            mpgdec_info->eof = TRUE;
        }
        else
            play_frame(&fr);
    }

    mpgdec_info->first_frame = FALSE;
    while (mpgdec_info->going) {
        if (mpgdec_info->jump_to_time != -1) {
            void *xp = NULL;
            if (have_xing_header)
                xp = &xing_header;
            if (mpgdec_seek(&fr, xp, vbr, mpgdec_info->jump_to_time) > -1) {
                mpgdec_ip.output->flush(mpgdec_info->jump_to_time * 1000);
                mpgdec_info->eof = FALSE;
            }
            mpgdec_info->jump_to_time = -1;
        }
        if (!mpgdec_info->eof) {
            if (mpgdec_read_frame(&fr) != 0) {
                if (fr.lay != mpgdec_layer || fr.lsf != mpgdec_lsf) {
                    memcpy(&temp_fr, &fr, sizeof(struct frame));
                    if (mpgdec_read_frame(&temp_fr) != 0) {
                        if (fr.lay == temp_fr.lay && fr.lsf == temp_fr.lsf) {
                            mpgdec_layer = fr.lay;
                            mpgdec_lsf = fr.lsf;
                            memcpy(&fr, &temp_fr, sizeof(struct frame));
                        }
                        else {
                            memcpy(&fr, &temp_fr, sizeof(struct frame));
                            skip_frames = 2;
                            mpgdec_info->output_audio = FALSE;
                            continue;
                        }

                    }
                }

                if (tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index] !=
                    mpgdec_bitrate)
                    mpgdec_bitrate =
                        tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];

                if (!disp_count) {
                    disp_count = 20;
                    if (mpgdec_bitrate != disp_bitrate) {
                        /* FIXME networks streams */
                        disp_bitrate = mpgdec_bitrate;
                        if (!have_xing_header
                            && strncasecmp(filename, "http://", 7)) {
                            double rel = mpgdec_relative_pos();
                            if (rel) {
                                mpgdec_length =
                                    mpgdec_ip.output->written_time() / rel;
                                vbr = TRUE;
                            }

                            if (rel == 0 || !(mpgdec_length > 0)) {
                                mpgdec_info->num_frames =
                                    mpgdec_calc_numframes(&fr);
                                mpgdec_info->tpf = mpgdec_compute_tpf(&fr);
                                mpgdec_length =
                                    mpgdec_info->num_frames *
                                    mpgdec_info->tpf * 1000;
                            }


                        }
                        mpgdec_ip.set_info(mpgdec_title, mpgdec_length,
                                           mpgdec_bitrate * 1000,
                                           mpgdec_frequency, mpgdec_stereo);
                    }
                }
                else
                    disp_count--;
                play_frame(&fr);
            }
            else {
                mpgdec_ip.output->buffer_free();
                mpgdec_ip.output->buffer_free();
                mpgdec_info->eof = TRUE;
                g_usleep(10000);
            }
        }
        else {
            g_usleep(10000);
        }
    }
    g_free(mpgdec_title);
    mpgdec_title = NULL;
    mpgdec_stream_close();
    if (output_opened && !audio_error)
        mpgdec_ip.output->close_audio();
    g_free(mpgdec_pcm_sample);
    mpgdec_filename = NULL;
    g_free(filename);

    return NULL;
}

static void
play_file(char *filename)
{
    memset(&fr, 0, sizeof(struct frame));
    memset(&temp_fr, 0, sizeof(struct frame));

    mpgdec_info = g_malloc0(sizeof(PlayerInfo));
    mpgdec_info->going = 1;
    mpgdec_info->first_frame = TRUE;
    mpgdec_info->output_audio = TRUE;
    mpgdec_info->jump_to_time = -1;
    skip_frames = 0;
    audio_error = FALSE;
    output_opened = FALSE;
    dopause = FALSE;

    decode_thread = g_thread_create(decode_loop, g_strdup(filename), TRUE,
                                    NULL);
}

static void
stop(void)
{
    if (mpgdec_info && mpgdec_info->going) {
        mpgdec_info->going = FALSE;
        g_thread_join(decode_thread);
        g_free(mpgdec_info);
        mpgdec_info = NULL;
    }
}

static void
seek(int time)
{
    mpgdec_info->jump_to_time = time;

    while (mpgdec_info->jump_to_time != -1)
        g_usleep(10000);
}

static void
do_pause(short p)
{
    if (output_opened)
        mpgdec_ip.output->pause(p);
    else
        dopause = p;
}

static int
get_time(void)
{
    if (audio_error)
        return -2;
    if (!mpgdec_info)
        return -1;
    if (!mpgdec_info->going
        || (mpgdec_info->eof && !mpgdec_ip.output->buffer_playing()))
        return -1;
    return mpgdec_ip.output->output_time();
}

static void
aboutbox(void)
{
    static GtkWidget *aboutbox;

    if (aboutbox != NULL)
        return;

    aboutbox = xmms_show_message(_("About MPEG Audio Plugin"),
				 _("Audacious decoding engine by William Pitcock <nenolod@nenolod.net>, derived from:\n"
                                   "mpg123 decoding engine by Michael Hipp <mh@mpg123.de>\n"
				   "Derived partially from mpg123 0.59s.mc3 as well.\n"
                                   "Based on the original XMMS plugin."),
                                  _("Ok"),
                                  FALSE, NULL, NULL);

    g_signal_connect(G_OBJECT(aboutbox), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &aboutbox);
}

InputPlugin mpgdec_ip = {
    NULL,
    NULL,
    NULL,                       /* Description */
    init,
    aboutbox,
    mpgdec_configure,
    is_our_file,
    NULL,
    play_file,
    stop,
    do_pause,
    seek,
    NULL,
    get_time,
    NULL, NULL,
    cleanup,
    NULL,
    NULL, NULL, NULL,
    get_song_info,
    mpgdec_file_info_box,       /* file_info_box */
    NULL,
    get_song_tuple
};

InputPlugin *
get_iplugin_info(void)
{
    mpgdec_ip.description = g_strdup_printf(_("MPEG Audio Plugin"));
    return &mpgdec_ip;
}
