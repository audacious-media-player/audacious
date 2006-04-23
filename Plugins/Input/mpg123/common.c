#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mpg123.h"

const int tabsel_123[2][3][16] = {
    {{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416,
      448,},
     {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,},
     {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320,}},
    {{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256,},
     {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,},
     {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,}}
};

const int mpg123_freqs[9] =
    { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000 };

struct bitstream_info bsi;

extern gint mpg123_bitrate, mpg123_frequency, mpg123_length;
extern gchar *mpg123_title, *mpg123_filename;
extern gboolean mpg123_stereo;

static int fsizeold = 0, ssize;
static unsigned char bsspace[2][MAXFRAMESIZE + 512];    /* MAXFRAMESIZE */
static unsigned char *bsbuf = bsspace[1], *bsbufold;
static int bsnum = 0;

unsigned char *mpg123_pcm_sample;
int mpg123_pcm_point = 0;

static VFSFile *filept;
static int filept_opened;

static int get_fileinfo(void);

static int
fullread(VFSFile * fd, unsigned char *buf, int count)
{
    int ret, cnt = 0;

    while (cnt < count) {
        if (fd)
            ret = vfs_fread(buf + cnt, 1, count - cnt, fd);
        else
            ret = mpg123_http_read(buf + cnt, count - cnt);
        if (ret < 0)
            return ret;
        if (ret == 0)
            break;
        cnt += ret;
    }
    return cnt;
}

static int
stream_init(void)
{
    if (get_fileinfo() < 0)
        return -1;
    return 0;
}

void
mpg123_stream_close(void)
{
    if (filept)
        vfs_fclose(filept);
    else if (mpg123_info->network_stream)
        mpg123_http_close();
}

/**************************************** 
 * HACK,HACK,HACK: step back <num> frames 
 * can only work if the 'stream' isn't a real stream but a file
static int stream_back_bytes(int bytes)
{
	if (vfs_fseek(filept, -bytes, SEEK_CUR) < 0)
		return -1;
	return 0;
}
 */

static int
stream_head_read(unsigned long *newhead)
{
    unsigned char hbuf[4];

    if (fullread(filept, hbuf, 4) != 4)
        return FALSE;

    *newhead = ((unsigned long) hbuf[0] << 24) |
        ((unsigned long) hbuf[1] << 16) |
        ((unsigned long) hbuf[2] << 8) | (unsigned long) hbuf[3];

    return TRUE;
}

static int
stream_head_shift(unsigned long *head)
{
    unsigned char hbuf;

    if (fullread(filept, &hbuf, 1) != 1)
        return 0;
    *head <<= 8;
    *head |= hbuf;
    *head &= 0xffffffff;
    return 1;
}

static int
stream_mpg123_read_frame_body(unsigned char *buf, int size)
{
    long l;

    if ((l = fullread(filept, buf, size)) != size) {
        if (l <= 0)
            return 0;
        memset(buf + l, 0, size - l);
    }
    return 1;
}

static long
stream_tell(void)
{
    return vfs_ftell(filept);
}

/*
static void stream_rewind(void)
{
	vfs_fseek(filept, 0, SEEK_SET);
}
*/

int
mpg123_stream_jump_to_frame(struct frame *fr, int frame)
{
    if (!filept)
        return -1;
    mpg123_read_frame_init();
    vfs_fseek(filept, frame * (fr->framesize + 4), SEEK_SET);
    mpg123_read_frame(fr);
    return 0;
}

int
mpg123_stream_jump_to_byte(struct frame *fr, int byte)
{
    if (!filept)
        return -1;
    vfs_fseek(filept, byte, SEEK_SET);
    mpg123_read_frame(fr);
    return 0;
}

int
mpg123_stream_check_for_xing_header(struct frame *fr, xing_header_t * xhead)
{
    unsigned char *head_data;
    int ret;

    vfs_fseek(filept, -(fr->framesize + 4), SEEK_CUR);
    head_data = g_malloc(fr->framesize + 4);
    vfs_fread(head_data, 1, fr->framesize + 4, filept);
    ret = mpg123_get_xing_header(xhead, head_data);
    g_free(head_data);
    return ret;
}

static int
get_fileinfo(void)
{
    guchar buf[3];

    if (filept == NULL)
        return -1;
    if (vfs_fseek(filept, 0, SEEK_END) < 0)
        return -1;

    mpg123_info->filesize = vfs_ftell(filept);
    if (vfs_fseek(filept, -128, SEEK_END) < 0)
        return -1;
    if (fullread(filept, buf, 3) != 3)
        return -1;
    if (!strncmp((char *) buf, "TAG", 3))
        mpg123_info->filesize -= 128;
    if (vfs_fseek(filept, 0, SEEK_SET) < 0)
        return -1;

    if (mpg123_info->filesize <= 0)
        return -1;

    return mpg123_info->filesize;
}

void
mpg123_read_frame_init(void)
{
    memset(bsspace[0], 0, MAXFRAMESIZE + 512);
    memset(bsspace[1], 0, MAXFRAMESIZE + 512);
    mpg123_info->output_audio = FALSE;
}

int
mpg123_head_check(unsigned long head)
{
    if ((head & 0xffe00000) != 0xffe00000)
        return FALSE;
    if (!((head >> 17) & 3))
        return FALSE;
    if (((head >> 12) & 0xf) == 0xf)
        return FALSE;
    if (!((head >> 12) & 0xf))
        return FALSE;
    if (((head >> 10) & 0x3) == 0x3)
        return FALSE;
    if (((head >> 19) & 1) == 1 &&
        ((head >> 17) & 3) == 3 && ((head >> 16) & 1) == 1)
        return FALSE;
    if ((head & 0xffff0000) == 0xfffe0000)
        return FALSE;

    return TRUE;
}

/*****************************************************************
 * read next frame
 */
int
mpg123_read_frame(struct frame *fr)
{
    unsigned long newhead;

    fsizeold = fr->framesize;   /* for Layer3 */

    if (!stream_head_read(&newhead))
        return FALSE;

    if (!mpg123_head_check(newhead) || !mpg123_decode_header(fr, newhead)) {
        int try = 0;

        do {
            try++;
            if ((newhead & 0xffffff00) ==
                ('I' << 24) + ('D' << 16) + ('3' << 8)) {
                if (!stream_head_read(&newhead))
                    return FALSE;
            }
            else if (!stream_head_shift(&newhead))
                return 0;

        }
        while ((!mpg123_head_check(newhead) ||
                !mpg123_decode_header(fr, newhead)) && try < (256 * 1024));
        if (try >= (256 * 1024))
            return FALSE;

        mpg123_info->filesize -= try;
    }
    /* flip/init buffer for Layer 3 */
    bsbufold = bsbuf;
    bsbuf = bsspace[bsnum] + 512;
    bsnum = (bsnum + 1) & 1;

    if (!stream_mpg123_read_frame_body(bsbuf, fr->framesize))
        return 0;

    bsi.bitindex = 0;
    bsi.wordpointer = (unsigned char *) bsbuf;


    return 1;

}

/*
 * the code a header and write the information
 * into the frame structure
 */
int
mpg123_decode_header(struct frame *fr, unsigned long newhead)
{
    if (newhead & (1 << 20)) {
        fr->lsf = (newhead & (1 << 19)) ? 0x0 : 0x1;
        fr->mpeg25 = 0;
    }
    else {
        fr->lsf = 1;
        fr->mpeg25 = 1;
    }
    fr->lay = 4 - ((newhead >> 17) & 3);
    if (fr->mpeg25) {
        fr->sampling_frequency = 6 + ((newhead >> 10) & 0x3);
    }
    else
        fr->sampling_frequency = ((newhead >> 10) & 0x3) + (fr->lsf * 3);
    fr->error_protection = ((newhead >> 16) & 0x1) ^ 0x1;

    fr->bitrate_index = ((newhead >> 12) & 0xf);
    fr->padding = ((newhead >> 9) & 0x1);
    fr->extension = ((newhead >> 8) & 0x1);
    fr->mode = ((newhead >> 6) & 0x3);
    fr->mode_ext = ((newhead >> 4) & 0x3);
    fr->copyright = ((newhead >> 3) & 0x1);
    fr->original = ((newhead >> 2) & 0x1);
    fr->emphasis = newhead & 0x3;

    fr->stereo = (fr->mode == MPG_MD_MONO) ? 1 : 2;

    ssize = 0;

    if (!fr->bitrate_index)
        return (0);

    switch (fr->lay) {
    case 1:
        fr->do_layer = mpg123_do_layer1;
        /* inits also shared tables with layer1 */
        mpg123_init_layer2(fr->synth_type == SYNTH_MMX);
        fr->framesize =
            (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
        fr->framesize /= mpg123_freqs[fr->sampling_frequency];
        fr->framesize = ((fr->framesize + fr->padding) << 2) - 4;
        break;
    case 2:
        fr->do_layer = mpg123_do_layer2;
        /* inits also shared tables with layer1 */
        mpg123_init_layer2(fr->synth_type == SYNTH_MMX);
        fr->framesize =
            (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
        fr->framesize /= mpg123_freqs[fr->sampling_frequency];
        fr->framesize += fr->padding - 4;
        break;
    case 3:
        fr->do_layer = mpg123_do_layer3;
        if (fr->lsf)
            ssize = (fr->stereo == 1) ? 9 : 17;
        else
            ssize = (fr->stereo == 1) ? 17 : 32;
        if (fr->error_protection)
            ssize += 2;
        fr->framesize =
            (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
        fr->framesize /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
        fr->framesize = fr->framesize + fr->padding - 4;
        break;
    default:
        return (0);
    }
    if (fr->framesize > MAXFRAMESIZE)
        return 0;
    return 1;
}

void
mpg123_open_stream(char *bs_filenam, int fd)
{
    filept_opened = 1;
    if (!strncasecmp(bs_filenam, "http://", 7)) {
        filept = NULL;
        mpg123_http_open(bs_filenam);
        mpg123_info->filesize = 0;
        mpg123_info->network_stream = TRUE;
    }
    else {
        if ((filept = vfs_fopen(bs_filenam, "rb")) == NULL ||
            stream_init() == -1)
            mpg123_info->eof = TRUE;
    }

}

void
mpg123_set_pointer(long backstep)
{
    bsi.wordpointer = bsbuf + ssize - backstep;
    if (backstep)
        memcpy(bsi.wordpointer, bsbufold + fsizeold - backstep, backstep);
    bsi.bitindex = 0;
}

double
mpg123_compute_bpf(struct frame *fr)
{
    double bpf;

    switch (fr->lay) {
    case 1:
        bpf = tabsel_123[fr->lsf][0][fr->bitrate_index];
        bpf *= 12000.0 * 4.0;
        bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
        break;
    case 2:
    case 3:
        bpf = tabsel_123[fr->lsf][fr->lay - 1][fr->bitrate_index];
        bpf *= 144000;
        bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
        break;
    default:
        bpf = 1.0;
    }

    return bpf;
}

int
mpg123_calc_numframes(struct frame *fr)
{
    return (int) (mpg123_info->filesize / mpg123_compute_bpf(fr));
}

double
mpg123_relative_pos(void)
{
    if (!filept || !mpg123_info->filesize)
        return 0;
    return ((double) stream_tell()) / mpg123_info->filesize;
}
