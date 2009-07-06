#ifndef AUDACIOUS_MD5_H
#define AUDACIOUS_MD5_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
    guint32 bits[2];    /* message length in bits, lsw first */
    guint32 buf[4];     /* digest buffer */
    guint8 in[64];      /* accumulate block */
} aud_md5state_t;

#define AUD_MD5HASH_LENGTH       (16)
#define AUD_MD5HASH_LENGTH_CH    (AUD_MD5HASH_LENGTH * 2)

typedef guint8 aud_md5hash_t[AUD_MD5HASH_LENGTH];


void aud_md5_init(aud_md5state_t *ctx);
void aud_md5_append(aud_md5state_t *ctx, const guint8 *buf, guint len);
void aud_md5_finish(aud_md5state_t *ctx, aud_md5hash_t digest);


G_END_DECLS

#endif /* AUDACIOUS_MD5_H */
