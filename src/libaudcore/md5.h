/**
 * @file md5.h
 * Functions for computing MD5 hashes of given data.
 */
#ifndef AUDACIOUS_MD5_H
#define AUDACIOUS_MD5_H

#include <glib.h>

G_BEGIN_DECLS

/** State context structure for MD5 hash calculation */
typedef struct {
    guint32 bits[2];    /**< Message length in bits, lsw first */
    guint32 buf[4];     /**< Message digest buffer */
    guint8 in[64];      /**< Data accumulation block */
} aud_md5state_t;

/** Length of MD5 hash in bytes */
#define AUD_MD5HASH_LENGTH       (16)

/** Length of MD5 hash in ASCII characters */
#define AUD_MD5HASH_LENGTH_CH    (AUD_MD5HASH_LENGTH * 2)

/** Type for holding calculated MD5 hash digest */
typedef guint8 aud_md5hash_t[AUD_MD5HASH_LENGTH];


void aud_md5_init(aud_md5state_t *ctx);
void aud_md5_append(aud_md5state_t *ctx, const guint8 *buf, guint len);
void aud_md5_finish(aud_md5state_t *ctx, aud_md5hash_t digest);


G_END_DECLS

#endif /* AUDACIOUS_MD5_H */
