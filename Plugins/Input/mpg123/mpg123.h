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

#include <gtk/gtk.h>

#include "audacious/plugin.h"
#include "dxhead.h"
#include "xmms-id3.h"

#define real float

#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

#define         ENCODING_SEPARATOR      " ,:;|/"

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
    guint32 filesize;           /* Filesize without junk */
} PlayerInfo;

void mpg123_set_eq(int on, float preamp, float *band);
void mpg123_file_info_box(char *);

extern PlayerInfo *mpg123_info;
extern InputPlugin mpg123_ip;

struct al_table {
    short bits;
    short d;
};

struct frame {
    struct al_table *alloc;
    int (*synth) (real *, int, unsigned char *, int *);
    int (*synth_mono) (real *, unsigned char *, int *);
#ifdef USE_SIMD
    void (*dct36) (real *, real *, real *, real *, real *);
#endif
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

void mpg123_configure(void);

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

extern MPG123Config mpg123_cfg;

struct bitstream_info {
    int bitindex;
    unsigned char *wordpointer;
};

extern struct bitstream_info bsi;

/* ------ Declarations from "http.c" ------ */

extern int mpg123_http_open(char *url);
int mpg123_http_read(gpointer data, gint length);
void mpg123_http_close(void);
char *mpg123_http_get_title(char *url);
int mpg123_http_get_length(void);
void mpg123_http_seek(long pos);

/* ------ Declarations from "common.c" ------ */
extern unsigned int mpg123_get1bit(void);
extern unsigned int mpg123_getbits(int);
extern unsigned int mpg123_getbits_fast(int);

extern void mpg123_open_stream(char *bs_filenam, int fd);
extern int mpg123_head_check(unsigned long);
extern void mpg123_stream_close(void);

extern void mpg123_set_pointer(long);

extern unsigned char *mpg123_pcm_sample;
extern int mpg123_pcm_point;

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
    real *full_gain[3];
    real *pow2gain;
};

struct III_sideinfo {
    unsigned main_data_begin;
    unsigned private_bits;
    struct {
        struct gr_info_s gr[2];
    } ch[2];
};

extern void open_stream(char *, int fd);
extern long mpg123_tell_stream(void);
extern void mpg123_read_frame_init(void);
extern int mpg123_read_frame(struct frame *fr);
extern int mpg123_back_frame(struct frame *fr, int num);
int mpg123_stream_jump_to_frame(struct frame *fr, int frame);
int mpg123_stream_jump_to_byte(struct frame *fr, int byte);
int mpg123_stream_check_for_xing_header(struct frame *fr,
                                        xing_header_t * xhead);
int mpg123_calc_numframes(struct frame *fr);

extern int mpg123_do_layer3(struct frame *fr);
extern int mpg123_do_layer2(struct frame *fr);
extern int mpg123_do_layer1(struct frame *fr);

extern int mpg123_synth_1to1(real *, int, unsigned char *, int *);
extern int mpg123_synth_1to1_8bit(real *, int, unsigned char *, int *);
extern int mpg123_synth_1to1_mono(real *, unsigned char *, int *);
extern int mpg123_synth_1to1_mono2stereo(real *, unsigned char *, int *);
extern int mpg123_synth_1to1_8bit_mono(real *, unsigned char *, int *);
extern int mpg123_synth_1to1_8bit_mono2stereo(real *, unsigned char *,
                                              int *);

extern int mpg123_synth_2to1(real *, int, unsigned char *, int *);
extern int mpg123_synth_2to1_8bit(real *, int, unsigned char *, int *);
extern int mpg123_synth_2to1_mono(real *, unsigned char *, int *);
extern int mpg123_synth_2to1_mono2stereo(real *, unsigned char *, int *);
extern int mpg123_synth_2to1_8bit_mono(real *, unsigned char *, int *);
extern int mpg123_synth_2to1_8bit_mono2stereo(real *, unsigned char *,
                                              int *);

extern int mpg123_synth_4to1(real *, int, unsigned char *, int *);
extern int mpg123_synth_4to1_8bit(real *, int, unsigned char *, int *);
extern int mpg123_synth_4to1_mono(real *, unsigned char *, int *);
extern int mpg123_synth_4to1_mono2stereo(real *, unsigned char *, int *);
extern int mpg123_synth_4to1_8bit_mono(real *, unsigned char *, int *);
extern int mpg123_synth_4to1_8bit_mono2stereo(real *, unsigned char *,
                                              int *);

extern void mpg123_rewindNbits(int bits);
extern int mpg123_hsstell(void);
extern void mpg123_set_pointer(long);
extern void mpg123_huffman_decoder(int, int *);
extern void mpg123_huffman_count1(int, int *);
extern int mpg123_get_songlen(struct frame *fr, int no);

#ifdef USE_SIMD
void mpg123_dct64_mmx(real *, real *, real *);
int mpg123_synth_1to1_mmx(real *, int, unsigned char *, int *);

void mpg123_dct36(real *, real *, real *, real *, real *);
void dct36_3dnow(real *, real *, real *, real *, real *);
int mpg123_synth_1to1_3dnow(real *, int, unsigned char *, int *);

int mpg123_getcpuflags(guint32 * fflags, guint32 * efflags);
#else
#define mpg123_getcpuflags(a, b)		\
do {						\
	*(a) = 0;				\
	*(b) = 0;				\
} while (0)
#endif

void mpg123_init_layer3(int);
void mpg123_init_layer2(gboolean);
void mpg123_make_decode_tables(long scaleval);
void mpg123_make_conv16to8_table(void);
void mpg123_dct64(real *, real *, real *);

int mpg123_decode_header(struct frame *fr, unsigned long newhead);
double mpg123_compute_bpf(struct frame *fr);
double mpg123_compute_tpf(struct frame *fr);
guint mpg123_strip_spaces(char *src, size_t n);
void mpg123_get_id3v2(struct id3_tag *id3d, struct id3tag_t *tag);
gchar *mpg123_format_song_title(struct id3tag_t *tag, gchar * filename);
double mpg123_relative_pos(void);


extern gchar ** mpg123_id3_encoding_list;
extern unsigned char *mpg123_conv16to8;
extern const int mpg123_freqs[9];
extern real mpg123_muls[27][64];
extern real mpg123_decwin[512 + 32];
extern real *mpg123_pnts[5];

#define GENRE_MAX 0x94
extern const char *mpg123_id3_genres[GENRE_MAX];
extern const int tabsel_123[2][3][16];

#endif
