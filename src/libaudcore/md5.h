/**
 * @file md5.h
 * Functions for computing MD5 hashes of given data.
 */
#ifndef LIBAUDCORE_MD5_H
#define LIBAUDCORE_MD5_H

#include <stdint.h>

/** State context structure for MD5 hash calculation */
typedef struct {
    uint32_t bits[2];    /**< Message length in bits, lsw first */
    uint32_t buf[4];     /**< Message digest buffer */
    uint8_t in[64];      /**< Data accumulation block */
} aud_md5state_t;

/** Length of MD5 hash in bytes */
#define AUD_MD5HASH_LENGTH       (16)

/** Length of MD5 hash in ASCII characters */
#define AUD_MD5HASH_LENGTH_CH    (AUD_MD5HASH_LENGTH * 2)

/** Type for holding calculated MD5 hash digest */
typedef uint8_t aud_md5hash_t[AUD_MD5HASH_LENGTH];


void aud_md5_init(aud_md5state_t *ctx);
void aud_md5_append(aud_md5state_t *ctx, const uint8_t *buf, unsigned int len);
void aud_md5_finish(aud_md5state_t *ctx, aud_md5hash_t digest);

#endif /* LIBAUDCORE_MD5_H */
