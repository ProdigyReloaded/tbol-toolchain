/*
 * Copyright 2025-2026, Phillip Heller
 *
 * This file is part of Prodigy Reloaded.
 *
 * Prodigy Reloaded is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Prodigy Reloaded is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Prodigy Reloaded. If not,
 * see <https://www.gnu.org/licenses/>.
 *
 *
 * Straightforward implementation of the MD5 message-digest algorithm
 * (RFC 1321).  Not used for any security purpose - we only need a
 * stable fingerprint to label decompilation output.
 */
#include "md5.h"
#include <string.h>

#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define STEP(f, a, b, c, d, x, t, s) \
    do { (a) += f((b), (c), (d)) + (x) + (t); \
         (a) = ROTL32((a), (s)); \
         (a) += (b); } while (0)

static void md5_compress(MD5Ctx *ctx, const uint8_t block[64]) {
    uint32_t M[16];
    for (int i = 0; i < 16; i++) {
        M[i] = (uint32_t)block[i*4]
             | ((uint32_t)block[i*4 + 1] << 8)
             | ((uint32_t)block[i*4 + 2] << 16)
             | ((uint32_t)block[i*4 + 3] << 24);
    }

    uint32_t a = ctx->state[0], b = ctx->state[1],
             c = ctx->state[2], d = ctx->state[3];

    /* Round 1 */
    STEP(F, a, b, c, d, M[ 0], 0xd76aa478,  7);
    STEP(F, d, a, b, c, M[ 1], 0xe8c7b756, 12);
    STEP(F, c, d, a, b, M[ 2], 0x242070db, 17);
    STEP(F, b, c, d, a, M[ 3], 0xc1bdceee, 22);
    STEP(F, a, b, c, d, M[ 4], 0xf57c0faf,  7);
    STEP(F, d, a, b, c, M[ 5], 0x4787c62a, 12);
    STEP(F, c, d, a, b, M[ 6], 0xa8304613, 17);
    STEP(F, b, c, d, a, M[ 7], 0xfd469501, 22);
    STEP(F, a, b, c, d, M[ 8], 0x698098d8,  7);
    STEP(F, d, a, b, c, M[ 9], 0x8b44f7af, 12);
    STEP(F, c, d, a, b, M[10], 0xffff5bb1, 17);
    STEP(F, b, c, d, a, M[11], 0x895cd7be, 22);
    STEP(F, a, b, c, d, M[12], 0x6b901122,  7);
    STEP(F, d, a, b, c, M[13], 0xfd987193, 12);
    STEP(F, c, d, a, b, M[14], 0xa679438e, 17);
    STEP(F, b, c, d, a, M[15], 0x49b40821, 22);

    /* Round 2 */
    STEP(G, a, b, c, d, M[ 1], 0xf61e2562,  5);
    STEP(G, d, a, b, c, M[ 6], 0xc040b340,  9);
    STEP(G, c, d, a, b, M[11], 0x265e5a51, 14);
    STEP(G, b, c, d, a, M[ 0], 0xe9b6c7aa, 20);
    STEP(G, a, b, c, d, M[ 5], 0xd62f105d,  5);
    STEP(G, d, a, b, c, M[10], 0x02441453,  9);
    STEP(G, c, d, a, b, M[15], 0xd8a1e681, 14);
    STEP(G, b, c, d, a, M[ 4], 0xe7d3fbc8, 20);
    STEP(G, a, b, c, d, M[ 9], 0x21e1cde6,  5);
    STEP(G, d, a, b, c, M[14], 0xc33707d6,  9);
    STEP(G, c, d, a, b, M[ 3], 0xf4d50d87, 14);
    STEP(G, b, c, d, a, M[ 8], 0x455a14ed, 20);
    STEP(G, a, b, c, d, M[13], 0xa9e3e905,  5);
    STEP(G, d, a, b, c, M[ 2], 0xfcefa3f8,  9);
    STEP(G, c, d, a, b, M[ 7], 0x676f02d9, 14);
    STEP(G, b, c, d, a, M[12], 0x8d2a4c8a, 20);

    /* Round 3 */
    STEP(H, a, b, c, d, M[ 5], 0xfffa3942,  4);
    STEP(H, d, a, b, c, M[ 8], 0x8771f681, 11);
    STEP(H, c, d, a, b, M[11], 0x6d9d6122, 16);
    STEP(H, b, c, d, a, M[14], 0xfde5380c, 23);
    STEP(H, a, b, c, d, M[ 1], 0xa4beea44,  4);
    STEP(H, d, a, b, c, M[ 4], 0x4bdecfa9, 11);
    STEP(H, c, d, a, b, M[ 7], 0xf6bb4b60, 16);
    STEP(H, b, c, d, a, M[10], 0xbebfbc70, 23);
    STEP(H, a, b, c, d, M[13], 0x289b7ec6,  4);
    STEP(H, d, a, b, c, M[ 0], 0xeaa127fa, 11);
    STEP(H, c, d, a, b, M[ 3], 0xd4ef3085, 16);
    STEP(H, b, c, d, a, M[ 6], 0x04881d05, 23);
    STEP(H, a, b, c, d, M[ 9], 0xd9d4d039,  4);
    STEP(H, d, a, b, c, M[12], 0xe6db99e5, 11);
    STEP(H, c, d, a, b, M[15], 0x1fa27cf8, 16);
    STEP(H, b, c, d, a, M[ 2], 0xc4ac5665, 23);

    /* Round 4 */
    STEP(I, a, b, c, d, M[ 0], 0xf4292244,  6);
    STEP(I, d, a, b, c, M[ 7], 0x432aff97, 10);
    STEP(I, c, d, a, b, M[14], 0xab9423a7, 15);
    STEP(I, b, c, d, a, M[ 5], 0xfc93a039, 21);
    STEP(I, a, b, c, d, M[12], 0x655b59c3,  6);
    STEP(I, d, a, b, c, M[ 3], 0x8f0ccc92, 10);
    STEP(I, c, d, a, b, M[10], 0xffeff47d, 15);
    STEP(I, b, c, d, a, M[ 1], 0x85845dd1, 21);
    STEP(I, a, b, c, d, M[ 8], 0x6fa87e4f,  6);
    STEP(I, d, a, b, c, M[15], 0xfe2ce6e0, 10);
    STEP(I, c, d, a, b, M[ 6], 0xa3014314, 15);
    STEP(I, b, c, d, a, M[13], 0x4e0811a1, 21);
    STEP(I, a, b, c, d, M[ 4], 0xf7537e82,  6);
    STEP(I, d, a, b, c, M[11], 0xbd3af235, 10);
    STEP(I, c, d, a, b, M[ 2], 0x2ad7d2bb, 15);
    STEP(I, b, c, d, a, M[ 9], 0xeb86d391, 21);

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
}

void md5_init(MD5Ctx *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->count = 0;
}

void md5_update(MD5Ctx *ctx, const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    size_t buf_pos = ctx->count % 64;
    ctx->count += len;

    if (buf_pos > 0 && buf_pos + len >= 64) {
        size_t fill = 64 - buf_pos;
        memcpy(ctx->buffer + buf_pos, p, fill);
        md5_compress(ctx, ctx->buffer);
        p += fill;
        len -= fill;
        buf_pos = 0;
    }

    while (len >= 64) {
        md5_compress(ctx, p);
        p += 64;
        len -= 64;
    }

    if (len > 0) {
        memcpy(ctx->buffer + buf_pos, p, len);
    }
}

void md5_finalize(MD5Ctx *ctx, uint8_t digest[16]) {
    uint64_t bit_count = ctx->count * 8;
    size_t buf_pos = ctx->count % 64;

    /* Append 0x80, pad with zeros, then 8-byte little-endian bit count. */
    ctx->buffer[buf_pos++] = 0x80;
    if (buf_pos > 56) {
        memset(ctx->buffer + buf_pos, 0, 64 - buf_pos);
        md5_compress(ctx, ctx->buffer);
        buf_pos = 0;
    }
    memset(ctx->buffer + buf_pos, 0, 56 - buf_pos);
    for (int i = 0; i < 8; i++)
        ctx->buffer[56 + i] = (uint8_t)(bit_count >> (i * 8));
    md5_compress(ctx, ctx->buffer);

    for (int i = 0; i < 4; i++) {
        digest[i*4    ] = (uint8_t)(ctx->state[i]      );
        digest[i*4 + 1] = (uint8_t)(ctx->state[i] >>  8);
        digest[i*4 + 2] = (uint8_t)(ctx->state[i] >> 16);
        digest[i*4 + 3] = (uint8_t)(ctx->state[i] >> 24);
    }
}

void md5_buffer(const void *data, size_t len, uint8_t digest[16]) {
    MD5Ctx ctx;
    md5_init(&ctx);
    md5_update(&ctx, data, len);
    md5_finalize(&ctx, digest);
}

void md5_hex(const uint8_t digest[16], char out[33]) {
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 16; i++) {
        out[i*2    ] = hex[digest[i] >> 4];
        out[i*2 + 1] = hex[digest[i] & 0x0F];
    }
    out[32] = '\0';
}
