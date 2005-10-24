/*
 * A Xing header may be present in the ancillary
 * data field of the first frame of an mp3 bitstream
 * The Xing header (optionally) contains
 *      frames      total number of audio frames in the bitstream
 *      bytes       total number of bytes in the bitstream
 *      toc         table of contents
 *
 * toc (table of contents) gives seek points
 * for random access
 * the ith entry determines the seek point for
 * i-percent duration
 * seek point in bytes = (toc[i]/256.0) * total_bitstream_bytes
 * e.g. half duration seek point = (toc[50]/256.0) * total_bitstream_bytes
 */

#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

/*
 * structure to receive extracted header
 */
typedef struct {
    int frames;                 /* total bit stream frames from Xing header data */
    int bytes;                  /* total bit stream bytes from Xing header data */
    unsigned char toc[100];     /* "table of contents" */
} xing_header_t;

/*
 * Returns zero on fail, non-zero on success
 * xing structure to receive header data (output)
 * buf bitstream input
 */
int mpg123_get_xing_header(xing_header_t * xing, unsigned char *buf);


/*
 * Returns seekpoint in bytes (may be at eof if percent=100.0)
 * percent: play time percentage of total playtime. May be fractional.
 */
int mpg123_seek_point(xing_header_t * xing, float percent);
