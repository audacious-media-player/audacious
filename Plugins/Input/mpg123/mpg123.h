/*
 * mpg123 defines 
 * used source: musicout.h from mpegaudio package
 */

#ifndef __MPG123_H__
#define __MPG123_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <tag_c.h>

#ifdef INTEGER_COMPILE

typedef long mpgdec_real;

# define REAL_RADIX             15
# define REAL_FACTOR            (32.0 * 1024.0)

# define REAL_PLUS_32767        ( 32767 << REAL_RADIX )
# define REAL_MINUS_32768       ( -32768 << REAL_RADIX )

# define DOUBLE_TO_REAL(x)      ((int)((x) * REAL_FACTOR))
# define REAL_TO_SHORT(x)       ((x) >> REAL_RADIX)
# define REAL_MUL(x, y)         (((long long)(x) * (long long)(y)) >> REAL_RADIX)

#else

typedef float mpgdec_real;

#endif

#ifndef DOUBLE_TO_REAL
# define DOUBLE_TO_REAL(x)      (x)
#endif
#ifndef REAL_TO_SHORT
# define REAL_TO_SHORT(x)       (x)
#endif
#ifndef REAL_PLUS_32767
# define REAL_PLUS_32767        32767.0
#endif
#ifndef REAL_MINUS_32768
# define REAL_MINUS_32768       -32768.0
#endif
#ifndef REAL_MUL
# define REAL_MUL(x, y)         ((x) * (y))
#endif

enum {
    SYNTH_AUTO,
    SYNTH_FPU,
    SYNTH_3DNOW,
    SYNTH_MMX,
};

enum {
    DETECT_EXTENSION,
    DETECT_CONTENT,
    DETECT_BOTH
};

enum {
    STREAM_FILE,
    STREAM_HTTP,
    STREAM_RTSP
};

#include <gtk/gtk.h>

#include "audacious/plugin.h"
#include "dxhead.h"
#include "xmms-id3.h"

#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

#define         ENCODING_SEPARATOR      " ,:;|/"

#define		MAXFRAMESIZE		4096

#ifdef HAVE_NEMESI
int mpgdec_rtsp_open(char *url);
int mpgdec_rtsp_read(gpointer data, gsize length);
void mpgdec_rtsp_close (void);
#define CHECK_STREAM(filename) \
    (!strncasecmp(filename, "http://", 7) ||\
     !strncasecmp(filename, "rtsp://", 7))
#else
#define CHECK_STREAM(filename) \
    (!strncasecmp(filename, "http://", 7))
#endif

struct id3v1tag_t {
    char tag[3];                /* always "TAG": defines ID3v1 tag 128 bytes before EOF */
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    union {
        struct {
            char comment[30];
        } v1_0;
        struct {
            char comment[28];
            char __zero;
            unsigned char track_number;
        } v1_1;
    } u;
    unsigned char genre;
};

struct id3tag_t {
    char title[64];
    char artist[64];
    char album[64];
    char comment[256];
    char genre[256];
    gint year;
    gint track_number;
};

typedef struct {
    int going, num_frames, eof, jump_to_time, eq_active;
    int songtime;
    double tpf;
    float eq_mul[576];
    gboolean output_audio, first_frame, network_stream;
    gint stream_type;
    guint32 filesize;           /* Filesize without junk */
} PlayerInfo;

void mpgdec_set_eq(int on, float preamp, float *band);
void mpgdec_file_info_box(char *);

extern PlayerInfo *mpgdec_info;
extern InputPlugin mpgdec_ip;

struct al_table {
    short bits;
    short d;
};

struct frame {
    struct al_table *alloc;
    int (*synth) (mpgdec_real *, int, unsigned char *, int *);
    int (*synth_mono) (mpgdec_real *, unsigned char *, int *);
    int stereo;
    int jsbound;
    int single;
    int II_sblimit;
    int down_sample_sblimit;
    int lsf;
    int mpeg25;
    int down_sample;
    int header_change;
    int lay;
    int (*do_layer) (struct frame * fr);
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
    int framesize;              /* computed framesize */
    int synth_type;
};

void mpgdec_configure(void);

typedef struct {
    gint resolution;
    gint channels;
    gint downsample;
    gint http_buffer_size;
    gint http_prebuffer;
    gboolean use_proxy;
    gchar *proxy_host;
    gint proxy_port;
    gboolean proxy_use_auth;
    gchar *proxy_user, *proxy_pass;
    gboolean save_http_stream;
    gchar *save_http_path;
    gboolean use_udp_channel;
    gchar *id3_format;
    gboolean title_override, disable_id3v2;
    gboolean title_encoding_enabled;
    gchar *title_encoding;
    int detect_by;
    int default_synth;
} MPG123Config;

extern MPG123Config mpgdec_cfg;

struct mpgdec_instance {
    void (*dump_audio)(struct frame *fr);
};

struct bitstream_info {
    int bitindex;
    unsigned char *wordpointer;
};

extern struct bitstream_info bsi;

typedef struct mpgdec_instance mpgdec_t;

struct mpstr {
  int bsize;
  int framesize;
  int fsizeold;
  struct frame fr;
 /* int (*do_layer)(struct mpstr *,struct frame *fr,int,struct audio_info_struct *); */
  unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
  mpgdec_real hybrid_block[2][2][SBLIMIT*SSLIMIT];
  int  hybrid_blc[2];
  unsigned long header;
  int bsnum;
  mpgdec_real synth_buffs[2][2][0x110];
  int  synth_bo;

  struct bitstream_info bsi;
};


#define AUSHIFT		3

/* ------ Declarations from "http.c" ------ */

extern int mpgdec_http_open(char *url);
int mpgdec_http_read(gpointer data, gsize length);
void mpgdec_http_close(void);
char *mpgdec_http_get_title(char *url);
int mpgdec_http_get_length(void);
void mpgdec_http_seek(long pos);

/* ------ Declarations from "common.c" ------ */
extern unsigned int mpgdec_get1bit(void);
extern unsigned int mpgdec_getbits(int);
extern unsigned int mpgdec_getbits_fast(int);

extern void mpgdec_open_stream(char *bs_filenam, int fd);
extern int mpgdec_head_check(unsigned long);
extern void mpgdec_stream_close(void);

extern void mpgdec_set_pointer(long);

extern unsigned char *mpgdec_pcm_sample;
extern int mpgdec_pcm_point;

struct gr_info_s {
    int scfsi;
    unsigned part2_3_length;
    unsigned big_values;
    unsigned scalefac_compress;
    unsigned block_type;
    unsigned mixed_block_flag;
    unsigned table_select[3];
    unsigned subblock_gain[3];
    unsigned maxband[3];
    unsigned maxbandl;
    unsigned maxb;
    unsigned region1start;
    unsigned region2start;
    unsigned preflag;
    unsigned scalefac_scale;
    unsigned count1table_select;
    mpgdec_real *full_gain[3];
    mpgdec_real *pow2gain;
};

struct III_sideinfo {
    unsigned main_data_begin;
    unsigned private_bits;
    struct {
        struct gr_info_s gr[2];
    } ch[2];
};

extern void open_stream(char *, int fd);
extern long mpgdec_tell_stream(void);
extern void mpgdec_read_frame_init(void);
extern int mpgdec_read_frame(struct frame *fr);
extern int mpgdec_back_frame(struct frame *fr, int num);
int mpgdec_stream_jump_to_frame(struct frame *fr, int frame);
int mpgdec_stream_jump_to_byte(struct frame *fr, int byte);
int mpgdec_stream_check_for_xing_header(struct frame *fr,
                                        xing_header_t * xhead);
int mpgdec_calc_numframes(struct frame *fr);

extern int mpgdec_do_layer3(struct frame *fr);
extern int mpgdec_do_layer2(struct frame *fr);
extern int mpgdec_do_layer1(struct frame *fr);

extern int mpgdec_synth_1to1(mpgdec_real *, int, unsigned char *, int *);
extern int mpgdec_synth_1to1_8bit(mpgdec_real *, int, unsigned char *, int *);
extern int mpgdec_synth_1to1_mono(mpgdec_real *, unsigned char *, int *);
extern int mpgdec_synth_1to1_mono2stereo(mpgdec_real *, unsigned char *, int *);
extern int mpgdec_synth_1to1_8bit_mono(mpgdec_real *, unsigned char *, int *);
extern int mpgdec_synth_1to1_8bit_mono2stereo(mpgdec_real *, unsigned char *,
                                              int *);

extern int mpgdec_synth_ntom(mpgdec_real *, int, unsigned char *, int *);
extern int mpgdec_synth_ntom_8bit(mpgdec_real *, int, unsigned char *, int *);
extern int mpgdec_synth_ntom_mono(mpgdec_real *, unsigned char *, int *);
extern int mpgdec_synth_ntom_mono2stereo(mpgdec_real *, unsigned char *, int *);
extern int mpgdec_synth_ntom_8bit_mono(mpgdec_real *, unsigned char *, int *);
extern int mpgdec_synth_ntom_8bit_mono2stereo(mpgdec_real *, unsigned char *,
                                              int *);
extern void mpgdec_synth_ntom_set_step(long, long);

extern void mpgdec_rewindNbits(int bits);
extern int mpgdec_hsstell(void);
extern void mpgdec_set_pointer(long);
extern void mpgdec_huffman_decoder(int, int *);
extern void mpgdec_huffman_count1(int, int *);
extern int mpgdec_get_songlen(struct frame *fr, int no);

#define mpgdec_getcpuflags(a, b)		\
do {						\
	*(a) = 0;				\
	*(b) = 0;				\
} while (0)

void mpgdec_init_layer3(int);
void mpgdec_init_layer2(gboolean);
void mpgdec_make_decode_tables(long scaleval);
void mpgdec_make_conv16to8_table(void);
void mpgdec_dct64(mpgdec_real *, mpgdec_real *, mpgdec_real *);

int mpgdec_decode_header(struct frame *fr, unsigned long newhead);
double mpgdec_compute_bpf(struct frame *fr);
double mpgdec_compute_tpf(struct frame *fr);
guint mpgdec_strip_spaces(char *src, size_t n);
double mpgdec_relative_pos(void);


extern gchar ** mpgdec_id3_encoding_list;
extern unsigned char *mpgdec_conv16to8;
extern const int mpgdec_freqs[9];
extern mpgdec_real mpgdec_muls[27][64];
extern mpgdec_real mpgdec_decwin[512 + 32];
extern mpgdec_real *mpgdec_pnts[5];

#define GENRE_MAX 0x94
extern const char *mpgdec_id3_genres[GENRE_MAX];
extern const int tabsel_123[2][3][16];

/* psycho.c defines */
void psycho_init(void);
int psycho_process(void *data, int length, int nch);

/* interface.c */
mpgdec_t *mpgdec_new(void);

#endif
