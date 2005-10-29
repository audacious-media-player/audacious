#ifndef __VORBIS_H__
#define __VORBIS_H__

#include <vorbis/vorbisfile.h>

#include "libaudacious/vfs.h"

extern ov_callbacks vorbis_callbacks;

void vorbis_configure(void);

void vorbis_file_info_box(char *filename);

char *convert_to_utf8(const char *string);
char *convert_from_utf8(const char *string);

typedef struct {
    gint http_buffer_size;
    gint http_prebuffer;
    gboolean use_proxy;
    gchar *proxy_host;
    gint proxy_port;
    gboolean proxy_use_auth;
    gchar *proxy_user, *proxy_pass;
    gboolean save_http_stream;
    gchar *save_http_path;
    gboolean tag_override;
    gchar *tag_format;
    gboolean use_anticlip;
    gboolean use_replaygain;
    gint replaygain_mode;
    gboolean use_booster;
} vorbis_config_t;

enum {
    REPLAYGAIN_MODE_TRACK,
    REPLAYGAIN_MODE_ALBUM,
    REPLAYGAIN_MODE_LAST
};

#endif                          /* __VORBIS_H__ */
