#ifndef __HTTP_H__
#define __HTTP_H__

gint vorbis_http_open(const gchar * url);
gint vorbis_http_read(gpointer data, gint length);
void vorbis_http_close(void);
gchar *vorbis_http_get_title(const gchar * url);
gint vorbis_http_get_ice_bitrate(void);

#endif                          /* __HTTP_H__ */
