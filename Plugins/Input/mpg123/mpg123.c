#include "mpg123.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include <libaudacious/util.h>
#include <libaudacious/configdb.h>
#include <libaudacious/vfs.h>
#include <libaudacious/titlestring.h>

#include "audacious/util.h"

static const long outscale = 32768;

static struct frame fr, temp_fr;

PlayerInfo *mpg123_info = NULL;
static GThread *decode_thread;

static gboolean audio_error = FALSE, output_opened = FALSE, dopause = FALSE;
gint mpg123_bitrate, mpg123_frequency, mpg123_length, mpg123_layer,
    mpg123_lsf;
gchar *mpg123_title = NULL, *mpg123_filename = NULL;
static int disp_bitrate, skip_frames = 0;
static int cpu_fflags, cpu_efflags;
gboolean mpg123_stereo, mpg123_mpeg25;
int mpg123_mode;

gchar **mpg123_id3_encoding_list = NULL;

const char *mpg123_id3_genres[GENRE_MAX] = {
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
mpg123_compute_tpf(struct frame *fr)
{
    const int bs[4] = { 0, 384, 1152, 1152 };
    double tpf;

    tpf = bs[fr->lay];
    tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
    return tpf;
}

static void
set_synth_functions(struct frame *fr)
{
    typedef int (*func) (real *, int, unsigned char *, int *);
    typedef int (*func_mono) (real *, unsigned char *, int *);
    typedef void (*func_dct36) (real *, real *, real *, real *, real *);

    int ds = fr->down_sample;
    int p8 = 0;

    static func funcs[][3] = {
        {mpg123_synth_1to1,
         mpg123_synth_2to1,
         mpg123_synth_4to1},
        {mpg123_synth_1to1_8bit,
         mpg123_synth_2to1_8bit,
         mpg123_synth_4to1_8bit},
    };

    static func_mono funcs_mono[2][4] = {
        {mpg123_synth_1to1_mono,
         mpg123_synth_2to1_mono,
         mpg123_synth_4to1_mono},
        {mpg123_synth_1to1_8bit_mono,
         mpg123_synth_2to1_8bit_mono,
         mpg123_synth_4to1_8bit_mono}
    };

    if (mpg123_cfg.resolution == 8)
        p8 = 1;
    fr->synth = funcs[p8][ds];
    fr->synth_mono = funcs_mono[p8][ds];
    fr->synth_type = SYNTH_FPU;

    if (p8) {
        mpg123_make_conv16to8_table();
    }
}

static void
init(void)
{
    ConfigDb *db;

    mpg123_make_decode_tables(outscale);

    mpg123_cfg.resolution = 16;
    mpg123_cfg.channels = 2;
    mpg123_cfg.downsample = 0;
    mpg123_cfg.http_buffer_size = 128;
    mpg123_cfg.http_prebuffer = 25;
    mpg123_cfg.proxy_port = 8080;
    mpg123_cfg.proxy_use_auth = FALSE;
    mpg123_cfg.proxy_user = NULL;
    mpg123_cfg.proxy_pass = NULL;
    mpg123_cfg.use_udp_channel = TRUE;
    mpg123_cfg.title_override = FALSE;
    mpg123_cfg.disable_id3v2 = FALSE;
    mpg123_cfg.detect_by = DETECT_EXTENSION;
    mpg123_cfg.default_synth = SYNTH_AUTO;

    mpg123_cfg.title_encoding_enabled = FALSE;
    mpg123_cfg.title_encoding = NULL;

    db = bmp_cfg_db_open();

    bmp_cfg_db_get_int(db, "MPG123", "resolution", &mpg123_cfg.resolution);
    bmp_cfg_db_get_int(db, "MPG123", "channels", &mpg123_cfg.channels);
    bmp_cfg_db_get_int(db, "MPG123", "downsample", &mpg123_cfg.downsample);
    bmp_cfg_db_get_int(db, "MPG123", "http_buffer_size",
                       &mpg123_cfg.http_buffer_size);
    bmp_cfg_db_get_int(db, "MPG123", "http_prebuffer",
                       &mpg123_cfg.http_prebuffer);
    bmp_cfg_db_get_bool(db, "MPG123", "save_http_stream",
                        &mpg123_cfg.save_http_stream);
    if (!bmp_cfg_db_get_string
        (db, "MPG123", "save_http_path", &mpg123_cfg.save_http_path))
        mpg123_cfg.save_http_path = g_strdup(g_get_home_dir());

    bmp_cfg_db_get_bool(db, "MPG123", "use_udp_channel",
                        &mpg123_cfg.use_udp_channel);

    bmp_cfg_db_get_bool(db, "MPG123", "use_proxy", &mpg123_cfg.use_proxy);
    if (!bmp_cfg_db_get_string
        (db, "MPG123", "proxy_host", &mpg123_cfg.proxy_host))
        mpg123_cfg.proxy_host = g_strdup("localhost");
    bmp_cfg_db_get_int(db, "MPG123", "proxy_port", &mpg123_cfg.proxy_port);
    bmp_cfg_db_get_bool(db, "MPG123", "proxy_use_auth",
                        &mpg123_cfg.proxy_use_auth);
    bmp_cfg_db_get_string(db, "MPG123", "proxy_user", &mpg123_cfg.proxy_user);
    bmp_cfg_db_get_string(db, "MPG123", "proxy_pass", &mpg123_cfg.proxy_pass);

    bmp_cfg_db_get_bool(db, "MPG123", "title_override",
                        &mpg123_cfg.title_override);
    bmp_cfg_db_get_bool(db, "MPG123", "disable_id3v2",
                        &mpg123_cfg.disable_id3v2);
    if (!bmp_cfg_db_get_string
        (db, "MPG123", "id3_format", &mpg123_cfg.id3_format))
        mpg123_cfg.id3_format = g_strdup("%p - %t");
    bmp_cfg_db_get_int(db, "MPG123", "detect_by", &mpg123_cfg.detect_by);
    bmp_cfg_db_get_int(db, "MPG123", "default_synth",
                       &mpg123_cfg.default_synth);

    bmp_cfg_db_get_bool(db, "MPG123", "title_encoding_enabled", &mpg123_cfg.title_encoding_enabled);
    bmp_cfg_db_get_string(db, "MPG123", "title_encoding", &mpg123_cfg.title_encoding);
    if (mpg123_cfg.title_encoding_enabled)
        mpg123_id3_encoding_list = g_strsplit_set(mpg123_cfg.title_encoding, ENCODING_SEPARATOR, 0);

    bmp_cfg_db_close(db);

    if (mpg123_cfg.resolution != 16 && mpg123_cfg.resolution != 8)
        mpg123_cfg.resolution = 16;

    mpg123_cfg.channels = CLAMP(mpg123_cfg.channels, 0, 2);
    mpg123_cfg.downsample = CLAMP(mpg123_cfg.downsample, 0, 2);
    mpg123_getcpuflags(&cpu_fflags, &cpu_efflags);
}

static void
cleanup(void)
{
    g_strfreev(mpg123_id3_encoding_list);
}

/* needed for is_our_file() */
static int
read_n_bytes(VFSFile * file, guint8 * buf, int n)
{

    if (vfs_fread(buf, 1, n, file) != n) {
        return FALSE;
    }
    return TRUE;
}

static guint32
convert_to_header(guint8 * buf)
{

    return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

static guint32
convert_to_long(guint8 * buf)
{

    return (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
}

static guint16
read_wav_id(char *filename)
{
    VFSFile *file;
    guint16 wavid;
    guint8 buf[4];
    guint32 head;
    long seek;

    if (!(file = vfs_fopen(filename, "rb"))) {  /* Could not open file */
        return 0;
    }
    if (!(read_n_bytes(file, buf, 4))) {
        vfs_fclose(file);
        return 0;
    }
    head = convert_to_header(buf);
    if (head == ('R' << 24) + ('I' << 16) + ('F' << 8) + 'F') { /* Found a riff -- maybe WAVE */
        if (vfs_fseek(file, 4, SEEK_CUR) != 0) {    /* some error occured */
            vfs_fclose(file);
            return 0;
        }
        if (!(read_n_bytes(file, buf, 4))) {
            vfs_fclose(file);
            return 0;
        }
        head = convert_to_header(buf);
        if (head == ('W' << 24) + ('A' << 16) + ('V' << 8) + 'E') { /* Found a WAVE */
            seek = 0;
            do {
/* we'll be looking for the fmt-chunk which comes before the data-chunk */
/* A chunk consists of an header identifier (4 bytes), the length of the chunk
   (4 bytes), and the chunkdata itself, padded to be an even number of bytes.
   We'll skip all chunks until we find the "data"-one which could contain
   mpeg-data */
                if (seek != 0) {
                    if (vfs_fseek(file, seek, SEEK_CUR) != 0) { /* some error occured */
                        vfs_fclose(file);
                        return 0;
                    }
                }
                if (!(read_n_bytes(file, buf, 4))) {
                    vfs_fclose(file);
                    return 0;
                }
                head = convert_to_header(buf);
                if (!(read_n_bytes(file, buf, 4))) {
                    vfs_fclose(file);
                    return 0;
                }
                seek = convert_to_long(buf);
                seek = seek + (seek % 2);   /* Has to be even (padding) */
                if (seek >= 2
                    && head == ('f' << 24) + ('m' << 16) + ('t' << 8) + ' ') {
                    if (!(read_n_bytes(file, buf, 2))) {
                        vfs_fclose(file);
                        return 0;
                    }
                    wavid = buf[0] + 256 * buf[1];
                    seek -= 2;
                    /* we could go on looking for
                       other things, but all we
                       wanted was the wavid */
                    vfs_fclose(file);
                    return wavid;
                }
            }
            while (head != ('d' << 24) + ('a' << 16) + ('t' << 8) + 'a');
            /* it's RIFF WAVE */
        }
        /* it's RIFF */
    }
    /* it's not even RIFF */
    vfs_fclose(file);
    return 0;
}

#define DET_BUF_SIZE 1024

static gboolean
mpg123_detect_by_content(char *filename)
{
    VFSFile *file;
    guchar tmp[4];
    guint32 head;
    struct frame fr;
    guchar buf[DET_BUF_SIZE];
    int in_buf, i;
    gboolean ret = FALSE;

    if ((file = vfs_fopen(filename, "rb")) == NULL)
        return FALSE;
    if (vfs_fread(tmp, 1, 4, file) != 4)
        goto done;
    head = convert_to_header(tmp);
    while (!mpg123_head_check(head)) {
        /*
         * The mpeg-stream can start anywhere in the file,
         * so we check the entire file
         */
        /* Optimize this */
        in_buf = vfs_fread(buf, 1, DET_BUF_SIZE, file);
        if (in_buf == 0)
            goto done;

        for (i = 0; i < in_buf; i++) {
            head <<= 8;
            head |= buf[i];
            if (mpg123_head_check(head)) {
                vfs_fseek(file, i + 1 - in_buf, SEEK_CUR);
                break;
            }
        }
    }
    if (mpg123_decode_header(&fr, head)) {
        /*
         * We found something which looks like a MPEG-header.
         * We check the next frame too, to be sure
         */

        if (vfs_fseek(file, fr.framesize, SEEK_CUR) != 0)
            goto done;
        if (vfs_fread(tmp, 1, 4, file) != 4)
            goto done;
        head = convert_to_header(tmp);
        if (mpg123_head_check(head) && mpg123_decode_header(&fr, head))
            ret = TRUE;
    }

  done:
    vfs_fclose(file);
    return ret;
}

static int
is_our_file(char *filename)
{
    char *ext;
    guint16 wavid;

    /* FIXME: wtf? */
    /* We assume all http:// (except those ending in .ogg) are mpeg --
     * why do we do that? */
    if (!strncasecmp(filename, "http://", 7)) { 
        ext = strrchr(filename, '.');
        if (ext) {
            if (!strncasecmp(ext, ".ogg", 4))
                return FALSE;
            if (!strncasecmp(ext, ".rm", 3) ||
                !strncasecmp(ext, ".ra", 3) ||
                !strncasecmp(ext, ".rpm", 4) || 
                !strncasecmp(ext, ".ram", 4))
                return FALSE;
        }
        return TRUE;
    }
    if (mpg123_cfg.detect_by == DETECT_CONTENT)
        return (mpg123_detect_by_content(filename));

    ext = strrchr(filename, '.');
    if (ext) {
        if (!strncasecmp(ext, ".mp2", 4) || !strncasecmp(ext, ".mp3", 4)) {
            return TRUE;
        }
        if (!strncasecmp(ext, ".wav", 4)) {
            wavid = read_wav_id(filename);
            if (wavid == 85 || wavid == 80) {   /* Microsoft says 80, files say 85... */
                return TRUE;
            }
        }
    }

    if (mpg123_cfg.detect_by == DETECT_BOTH)
        return (mpg123_detect_by_content(filename));
    return FALSE;
}

static void
play_frame(struct frame *fr)
{
    if (fr->error_protection) {
        bsi.wordpointer += 2;
        /*  mpg123_getbits(16); *//* skip crc */
    }
    if (!fr->do_layer(fr)) {
        skip_frames = 2;
        mpg123_info->output_audio = FALSE;
    }
    else {
        if (!skip_frames)
            mpg123_info->output_audio = TRUE;
        else
            skip_frames--;
    }
}

static const char *
get_id3_genre(unsigned char genre_code)
{
    if (genre_code < GENRE_MAX)
        return gettext(mpg123_id3_genres[genre_code]);

    return "";
}

guint
mpg123_strip_spaces(char *src, size_t n)
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
mpg123_id3v1_to_id3v2(struct id3v1tag_t *v1, struct id3tag_t *v2)
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
    v2->year = atoi(v1->year);

    /* Check for v1.1 tags. */
    if (v1->u.v1_1.__zero == 0)
        v2->track_number = v1->u.v1_1.track_number;
    else
        v2->track_number = 0;
}

static char *
mpg123_getstr(char *str)
{
    if (str && strlen(str) > 0)
        return str;
    return NULL;
}

static gchar *
convert_id3_title(gchar * title)
{
    gchar **encoding = mpg123_id3_encoding_list;
    gchar *new_title = NULL;

    if (g_utf8_validate(title, -1, NULL))
        return title;

    while (*encoding && !new_title) {
        new_title = g_convert(title, strlen(title), "UTF-8", *encoding++,
                              NULL, NULL, NULL);
    }

    if (new_title) {
        g_free(title);
        return new_title;
    }

    /* FIXME: We're relying on BMP core to provide fallback
     * conversion */
    return title;
}

/*
 * Function mpg123_format_song_title (tag, filename)
 *
 *    Create song title according to `tag' and/or `filename' and
 *    return it.  The title must be subsequently freed using g_free().
 *
 */
gchar *
mpg123_format_song_title(struct id3tag_t * tag, gchar * filename)
{
    gchar *title = NULL;
    TitleInput *input;

    input = bmp_title_input_new();

    if (tag) {
        input->performer = mpg123_getstr(tag->artist);
        input->album_name = mpg123_getstr(tag->album);
        input->track_name = mpg123_getstr(tag->title);
        input->year = tag->year;
        input->track_number = tag->track_number;
        input->genre = mpg123_getstr(tag->genre);
        input->comment = mpg123_getstr(tag->comment);
    }

    input->file_name = g_path_get_basename(filename);
    input->file_path = g_path_get_dirname(filename);
    input->file_ext = extname(filename);

    title = xmms_get_titlestring(mpg123_cfg.title_override ?
                                 mpg123_cfg.id3_format :
                                 xmms_get_gentitle_format(), input);

    if (!title) {
        /* Format according to filename.  */
        title = g_path_get_basename(filename);
        if (extname(title))
            *(extname(title) - 1) = '\0';   /* removes period */
    }

    g_free(input->file_path);
    g_free(input->file_name);
    g_free(input);

    if (mpg123_cfg.title_encoding_enabled)
        title = convert_id3_title(title);

    return title;
}

/*
 * Function mpg123_get_id3v2 (id3d, tag)
 *
 *    Get desired contents from the indicated id3tag and store it in
 *    `tag'. 
 *
 */
void
mpg123_get_id3v2(struct id3_tag *id3d, struct id3tag_t *tag)
{
    struct id3_frame *id3frm;
    gchar *txt;
    gint tlen, num;

#define ID3_SET(_tid,_fld)                                              \
{                                                                       \
        id3frm = id3_get_frame( id3d, _tid, 1 );                        \
        if (id3frm) {                                                   \
                txt = _tid == ID3_TCON ? id3_get_content(id3frm)        \
                    : id3_get_text(id3frm);                             \
                if(txt)                                                 \
                {                                                       \
                        tlen = strlen(txt);                             \
                        if ( tlen >= sizeof(tag->_fld) )                \
                                tlen = sizeof(tag->_fld)-1;             \
                        strncpy( tag->_fld, txt, tlen );                \
                        tag->_fld[tlen] = 0;                            \
                        g_free(txt);                                    \
                }                                                       \
                else                                                    \
                        tag->_fld[0] = 0;                               \
        } else {                                                        \
                tag->_fld[0] = 0;                                       \
        }                                                               \
}

#define ID3_SET_NUM(_tid,_fld)                          \
{                                                       \
        id3frm = id3_get_frame(id3d, _tid, 1);          \
        if (id3frm) {                                   \
                num = id3_get_text_number(id3frm);      \
                tag->_fld = num >= 0 ? num : 0;         \
        } else                                          \
                tag->_fld = 0;                          \
}

    ID3_SET(ID3_TIT2, title);
    ID3_SET(ID3_TPE1, artist);
    if (strlen(tag->artist) == 0)
        ID3_SET(ID3_TPE2, artist);
    ID3_SET(ID3_TALB, album);
    ID3_SET_NUM(ID3_TYER, year);
    ID3_SET_NUM(ID3_TRCK, track_number);
    ID3_SET(ID3_COMM, comment);
    ID3_SET(ID3_TCON, genre);
}


/*
 * Function get_song_title (fd, filename)
 *
 *    Get song title of file.  File position of `fd' will be
 *    clobbered.  `fd' may be NULL, in which case `filename' is opened
 *    separately.  The returned song title must be subsequently freed
 *    using g_free().
 *
 */
static gchar *
get_song_title(VFSFile * fd, char *filename)
{
    VFSFile *file = fd;
    char *ret = NULL;
    struct id3v1tag_t id3v1tag;
    struct id3tag_t id3tag;

    if (file || (file = vfs_fopen(filename, "rb")) != 0) {
        struct id3_tag *id3 = NULL;

        /*
         * Try reading ID3v2 tag.
         */
        if (!mpg123_cfg.disable_id3v2) {
            vfs_fseek(file, 0, SEEK_SET);
            id3 = id3_open_fp(file, 0);
            if (id3) {
                mpg123_get_id3v2(id3, &id3tag);
                ret = mpg123_format_song_title(&id3tag, filename);
                id3_close(id3);
            }
        }

        /*
         * Try reading ID3v1 tag.
         */
        if (!id3 && (vfs_fseek(file, -1 * sizeof(id3v1tag), SEEK_END) == 0) &&
            (vfs_fread(&id3v1tag, 1, sizeof(id3v1tag), file) ==
             sizeof(id3v1tag)) && (strncmp(id3v1tag.tag, "TAG", 3) == 0)) {
            mpg123_id3v1_to_id3v2(&id3v1tag, &id3tag);
            ret = mpg123_format_song_title(&id3tag, filename);
        }

        if (!fd)
            /*
             * File was opened in this function.
             */
            vfs_fclose(file);
    }

    if (ret == NULL)
        /*
         * Unable to get ID3 tag.
         */
        ret = mpg123_format_song_title(NULL, filename);

    return ret;
}

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
    while (!mpg123_head_check(head)) {
        head <<= 8;
        if (vfs_fread(tmp, 1, 1, file) != 1)
            return 0;
        head |= tmp[0];
    }
    if (mpg123_decode_header(&frm, head)) {
        buf = g_malloc(frm.framesize + 4);
        vfs_fseek(file, -4, SEEK_CUR);
        vfs_fread(buf, 1, frm.framesize + 4, file);
        tpf = mpg123_compute_tpf(&frm);
        if (mpg123_get_xing_header(&xing_header, buf)) {
            g_free(buf);
            if (xing_header.bytes == 0)
                xing_header.bytes = get_song_length(file);
            return (tpf * xing_header.frames * 1000);
        }
        g_free(buf);
        bpf = mpg123_compute_bpf(&frm);
        len = get_song_length(file);
        return ((guint) (len / bpf) * tpf * 1000);
    }
    return 0;
}

static void
get_song_info(char *filename, char **title_real, int *len_real)
{
    VFSFile *file;

    (*len_real) = -1;
    (*title_real) = NULL;

    /*
     * TODO: Getting song info from http streams.
     */
    if (!strncasecmp(filename, "http://", 7))
        return;

    if ((file = vfs_fopen(filename, "rb")) != NULL) {
        (*len_real) = get_song_time(file);
        (*title_real) = get_song_title(file, filename);
        vfs_fclose(file);
    }
}

static int
open_output(void)
{
    int r;
    AFormat fmt = mpg123_cfg.resolution == 16 ? FMT_S16_NE : FMT_U8;
    int freq = mpg123_freqs[fr.sampling_frequency] >> mpg123_cfg.downsample;
    int channels = mpg123_cfg.channels == 2 ? fr.stereo : 1;
    r = mpg123_ip.output->open_audio(fmt, freq, channels);

    if (r && dopause) {
        mpg123_ip.output->pause(TRUE);
        dopause = FALSE;
    }

    return r;
}


static int
mpg123_seek(struct frame *fr, xing_header_t * xh, gboolean vbr, int time)
{
    int jumped = -1;

    if (xh) {
        int percent = ((double) time * 100.0) /
            (mpg123_info->num_frames * mpg123_info->tpf);
        int byte = mpg123_seek_point(xh, percent);
        jumped = mpg123_stream_jump_to_byte(fr, byte);
    }
    else if (vbr && mpg123_length > 0) {
        int byte = ((guint64) time * 1000 * mpg123_info->filesize) /
            mpg123_length;
        jumped = mpg123_stream_jump_to_byte(fr, byte);
    }
    else {
        int frame = time / mpg123_info->tpf;
        jumped = mpg123_stream_jump_to_frame(fr, frame);
    }

    return jumped;
}


static void *
decode_loop(void *arg)
{
    gboolean have_xing_header = FALSE, vbr = FALSE;
    int disp_count = 0, temp_time;
    char *filename = arg;
    xing_header_t xing_header;

    /* This is used by fileinfo on http streams */
    mpg123_bitrate = 0;

    mpg123_pcm_sample = g_malloc0(32768);
    mpg123_pcm_point = 0;
    mpg123_filename = filename;

    mpg123_read_frame_init();

    mpg123_open_stream(filename, -1);

    if (mpg123_info->eof || !mpg123_read_frame(&fr))
        mpg123_info->eof = TRUE;

    if (!mpg123_info->eof && mpg123_info->going) {
        if (mpg123_cfg.channels == 2)
            fr.single = -1;
        else
            fr.single = 3;

        fr.down_sample = mpg123_cfg.downsample;
        fr.down_sample_sblimit = SBLIMIT >> mpg123_cfg.downsample;
        set_synth_functions(&fr);
        mpg123_init_layer3(fr.down_sample_sblimit);

        mpg123_info->tpf = mpg123_compute_tpf(&fr);
        if (strncasecmp(filename, "http://", 7)) {
            if (mpg123_stream_check_for_xing_header(&fr, &xing_header)) {
                mpg123_info->num_frames = xing_header.frames;
                have_xing_header = TRUE;
                mpg123_read_frame(&fr);
            }
        }

        for (;;) {
            memcpy(&temp_fr, &fr, sizeof(struct frame));
            if (!mpg123_read_frame(&temp_fr)) {
                mpg123_info->eof = TRUE;
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
            mpg123_info->num_frames = mpg123_calc_numframes(&fr);

        memcpy(&fr, &temp_fr, sizeof(struct frame));
        mpg123_bitrate = tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];
        disp_bitrate = mpg123_bitrate;
        mpg123_frequency = mpg123_freqs[fr.sampling_frequency];
        mpg123_stereo = fr.stereo;
        mpg123_layer = fr.lay;
        mpg123_lsf = fr.lsf;
        mpg123_mpeg25 = fr.mpeg25;
        mpg123_mode = fr.mode;

        if (strncasecmp(filename, "http://", 7)) {
            mpg123_length = mpg123_info->num_frames * mpg123_info->tpf * 1000;
            if (!mpg123_title)
                mpg123_title = get_song_title(NULL, filename);
        }
        else {
            if (!mpg123_title)
                mpg123_title = mpg123_http_get_title(filename);
            mpg123_length = -1;
        }

        mpg123_ip.set_info(mpg123_title, mpg123_length,
                           mpg123_bitrate * 1000,
                           mpg123_freqs[fr.sampling_frequency], fr.stereo);

        output_opened = TRUE;

        if (!open_output()) {
            audio_error = TRUE;
            mpg123_info->eof = TRUE;
        }
        else
            play_frame(&fr);
    }

    mpg123_info->first_frame = FALSE;
    while (mpg123_info->going) {
        if (mpg123_info->jump_to_time != -1) {
            void *xp = NULL;
            if (have_xing_header)
                xp = &xing_header;
            if (mpg123_seek(&fr, xp, vbr, mpg123_info->jump_to_time) > -1) {
                mpg123_ip.output->flush(mpg123_info->jump_to_time * 1000);
                mpg123_info->eof = FALSE;
            }
            mpg123_info->jump_to_time = -1;
        }
        if (!mpg123_info->eof) {
            if (mpg123_read_frame(&fr) != 0) {
                if (fr.lay != mpg123_layer || fr.lsf != mpg123_lsf) {
                    memcpy(&temp_fr, &fr, sizeof(struct frame));
                    if (mpg123_read_frame(&temp_fr) != 0) {
                        if (fr.lay == temp_fr.lay && fr.lsf == temp_fr.lsf) {
                            mpg123_layer = fr.lay;
                            mpg123_lsf = fr.lsf;
                            memcpy(&fr, &temp_fr, sizeof(struct frame));
                        }
                        else {
                            memcpy(&fr, &temp_fr, sizeof(struct frame));
                            skip_frames = 2;
                            mpg123_info->output_audio = FALSE;
                            continue;
                        }

                    }
                }
                if (mpg123_freqs[fr.sampling_frequency] != mpg123_frequency
                    || mpg123_stereo != fr.stereo) {
                    memcpy(&temp_fr, &fr, sizeof(struct frame));
                    if (mpg123_read_frame(&temp_fr) != 0) {
                        if (fr.sampling_frequency ==
                            temp_fr.sampling_frequency
                            && temp_fr.stereo == fr.stereo) {
                            mpg123_ip.output->buffer_free();
                            mpg123_ip.output->buffer_free();
                            while (mpg123_ip.output->buffer_playing()
                                   && mpg123_info->going
                                   && mpg123_info->jump_to_time == -1)
                                xmms_usleep(20000);
                            if (!mpg123_info->going)
                                break;
                            temp_time = mpg123_ip.output->output_time();
                            mpg123_ip.output->close_audio();
                            mpg123_frequency =
                                mpg123_freqs[fr.sampling_frequency];
                            mpg123_stereo = fr.stereo;
                            if (!mpg123_ip.output->
                                open_audio(mpg123_cfg.resolution ==
                                           16 ? FMT_S16_NE : FMT_U8,
                                           mpg123_freqs[fr.sampling_frequency]
                                           >> mpg123_cfg.downsample,
                                           mpg123_cfg.channels ==
                                           2 ? fr.stereo : 1)) {
                                audio_error = TRUE;
                                mpg123_info->eof = TRUE;
                            }
                            mpg123_ip.output->flush(temp_time);
                            mpg123_ip.set_info(mpg123_title, mpg123_length,
                                               mpg123_bitrate * 1000,
                                               mpg123_frequency,
                                               mpg123_stereo);
                            memcpy(&fr, &temp_fr, sizeof(struct frame));
                        }
                        else {
                            memcpy(&fr, &temp_fr, sizeof(struct frame));
                            skip_frames = 2;
                            mpg123_info->output_audio = FALSE;
                            continue;
                        }
                    }
                }

                if (tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index] !=
                    mpg123_bitrate)
                    mpg123_bitrate =
                        tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];

                if (!disp_count) {
                    disp_count = 20;
                    if (mpg123_bitrate != disp_bitrate) {
                        /* FIXME networks streams */
                        disp_bitrate = mpg123_bitrate;
                        if (!have_xing_header
                            && strncasecmp(filename, "http://", 7)) {
                            double rel = mpg123_relative_pos();
                            if (rel) {
                                mpg123_length =
                                    mpg123_ip.output->written_time() / rel;
                                vbr = TRUE;
                            }

                            if (rel == 0 || !(mpg123_length > 0)) {
                                mpg123_info->num_frames =
                                    mpg123_calc_numframes(&fr);
                                mpg123_info->tpf = mpg123_compute_tpf(&fr);
                                mpg123_length =
                                    mpg123_info->num_frames *
                                    mpg123_info->tpf * 1000;
                            }


                        }
                        mpg123_ip.set_info(mpg123_title, mpg123_length,
                                           mpg123_bitrate * 1000,
                                           mpg123_frequency, mpg123_stereo);
                    }
                }
                else
                    disp_count--;
                play_frame(&fr);
            }
            else {
                mpg123_ip.output->buffer_free();
                mpg123_ip.output->buffer_free();
                mpg123_info->eof = TRUE;
                xmms_usleep(10000);
            }
        }
        else {
            xmms_usleep(10000);
        }
    }
    g_free(mpg123_title);
    mpg123_title = NULL;
    mpg123_stream_close();
    if (output_opened && !audio_error)
        mpg123_ip.output->close_audio();
    g_free(mpg123_pcm_sample);
    mpg123_filename = NULL;
    g_free(filename);

    return NULL;
}

static void
play_file(char *filename)
{
    memset(&fr, 0, sizeof(struct frame));
    memset(&temp_fr, 0, sizeof(struct frame));

    mpg123_info = g_malloc0(sizeof(PlayerInfo));
    mpg123_info->going = 1;
    mpg123_info->first_frame = TRUE;
    mpg123_info->output_audio = TRUE;
    mpg123_info->jump_to_time = -1;
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
    if (mpg123_info && mpg123_info->going) {
        mpg123_info->going = FALSE;
        g_thread_join(decode_thread);
        g_free(mpg123_info);
        mpg123_info = NULL;
    }
}

static void
seek(int time)
{
    mpg123_info->jump_to_time = time;

    while (mpg123_info->jump_to_time != -1)
        xmms_usleep(10000);
}

static void
do_pause(short p)
{
    if (output_opened)
        mpg123_ip.output->pause(p);
    else
        dopause = p;
}

static int
get_time(void)
{
    if (audio_error)
        return -2;
    if (!mpg123_info)
        return -1;
    if (!mpg123_info->going
        || (mpg123_info->eof && !mpg123_ip.output->buffer_playing()))
        return -1;
    return mpg123_ip.output->output_time();
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

InputPlugin mpg123_ip = {
    NULL,
    NULL,
    NULL,                       /* Description */
    init,
    aboutbox,
    mpg123_configure,
    is_our_file,
    NULL,
    play_file,
    stop,
    do_pause,
    seek,
    mpg123_set_eq,
    get_time,
    NULL, NULL,
    cleanup,
    NULL,
    NULL, NULL, NULL,
    get_song_info,
    mpg123_file_info_box,       /* file_info_box */
    NULL
};

InputPlugin *
get_iplugin_info(void)
{
    mpg123_ip.description = g_strdup_printf(_("MPEG Audio Plugin"));
    return &mpg123_ip;
}
