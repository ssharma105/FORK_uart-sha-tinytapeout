/*
    sha1.hpp - source code of

    ============
    SHA-1 in C++
    ============

    100% Public Domain.

    Original C Code
        -- Steve Reid <steve@edmweb.com>
    Small changes to fit into bglibs
        -- Bruce Guenter <bruce@untroubled.org>
    Translation to simpler C++ Code
        -- Volker Diels-Grabsch <v@njh.eu>
    Safety fixes
        -- Eugene Hopkinson <slowriot at voxelstorm dot com>
    Header-only library
        -- Zlatko Michailov <zlatko@michailov.org>
*/

#ifndef SHA1_HPP
#define SHA1_HPP

#include "preamble.h"

#include <array>
#include <cstdint>
#include <sstream>
#include <string_view>

class SHA1 {
   public:
    SHA1();
    void update(const std::string& s);
    void update(std::istream& is);
    std::array<u8, 20> final();

   private:
    u32 digest[5];
    std::string buffer;
    u64 transforms;
};

// number of 32bit integers per SHA1 block
static const size_t BLOCK_INTS = 16;
static const size_t BLOCK_BYTES = BLOCK_INTS * 4;

inline static void reset(u32 digest[], std::string& buffer, u64& transforms) {
    // SHA1 initialization constants
    digest[0] = 0x6745'2301;
    digest[1] = 0xefcd'ab89;
    digest[2] = 0x98ba'dcfe;
    digest[3] = 0x1032'5476;
    digest[4] = 0xc3d2'e1f0;

    // Reset counters
    buffer = "";
    transforms = 0;
}

inline static u32 rol(const u32 value, const size_t bits) {
    return (value << bits) | (value >> (32 - bits));
}

inline static u32 blk(const u32 block[BLOCK_INTS], const size_t i) {
    return rol(
        block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ block[(i + 2) & 15]
            ^ block[i],
        1
    );
}

/*
 * (R0+R1), R2, R3, R4 are the different operations used in SHA1
 */

inline static void
R0(const u32 block[BLOCK_INTS],
   const u32 v,
   u32& w,
   const u32 x,
   const u32 y,
   u32& z,
   const size_t i) {
    z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a82'7999 + rol(v, 5);
    w = rol(w, 30);
}

inline static void
R1(u32 block[BLOCK_INTS],
   const u32 v,
   u32& w,
   const u32 x,
   const u32 y,
   u32& z,
   const size_t i) {
    block[i] = blk(block, i);
    z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a82'7999 + rol(v, 5);
    w = rol(w, 30);
}

inline static void
R2(u32 block[BLOCK_INTS],
   const u32 v,
   u32& w,
   const u32 x,
   const u32 y,
   u32& z,
   const size_t i) {
    block[i] = blk(block, i);
    z += (w ^ x ^ y) + block[i] + 0x6ed9'eba1 + rol(v, 5);
    w = rol(w, 30);
}

inline static void
R3(u32 block[BLOCK_INTS],
   const u32 v,
   u32& w,
   const u32 x,
   const u32 y,
   u32& z,
   const size_t i) {
    block[i] = blk(block, i);
    z += (((w | x) & y) | (w & x)) + block[i] + 0x8f1b'bcdc + rol(v, 5);
    w = rol(w, 30);
}

inline static void
R4(u32 block[BLOCK_INTS],
   const u32 v,
   u32& w,
   const u32 x,
   const u32 y,
   u32& z,
   const size_t i) {
    block[i] = blk(block, i);
    z += (w ^ x ^ y) + block[i] + 0xca62'c1d6 + rol(v, 5);
    w = rol(w, 30);
}

/*
 * Hash a single 512-bit block. This is the core of the algorithm.
 */

inline static void transform(
    u32 digest[], u32 block[BLOCK_INTS], u64& transforms
) {
    /* Copy digest[] to working vars */
    u32 a = digest[0];
    u32 b = digest[1];
    u32 c = digest[2];
    u32 d = digest[3];
    u32 e = digest[4];

    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(block, a, b, c, d, e, 0);
    R0(block, e, a, b, c, d, 1);
    R0(block, d, e, a, b, c, 2);
    R0(block, c, d, e, a, b, 3);
    R0(block, b, c, d, e, a, 4);
    R0(block, a, b, c, d, e, 5);
    R0(block, e, a, b, c, d, 6);
    R0(block, d, e, a, b, c, 7);
    R0(block, c, d, e, a, b, 8);
    R0(block, b, c, d, e, a, 9);
    R0(block, a, b, c, d, e, 10);
    R0(block, e, a, b, c, d, 11);
    R0(block, d, e, a, b, c, 12);
    R0(block, c, d, e, a, b, 13);
    R0(block, b, c, d, e, a, 14);
    R0(block, a, b, c, d, e, 15);
    R1(block, e, a, b, c, d, 0);
    R1(block, d, e, a, b, c, 1);
    R1(block, c, d, e, a, b, 2);
    R1(block, b, c, d, e, a, 3);
    R2(block, a, b, c, d, e, 4);
    R2(block, e, a, b, c, d, 5);
    R2(block, d, e, a, b, c, 6);
    R2(block, c, d, e, a, b, 7);
    R2(block, b, c, d, e, a, 8);
    R2(block, a, b, c, d, e, 9);
    R2(block, e, a, b, c, d, 10);
    R2(block, d, e, a, b, c, 11);
    R2(block, c, d, e, a, b, 12);
    R2(block, b, c, d, e, a, 13);
    R2(block, a, b, c, d, e, 14);
    R2(block, e, a, b, c, d, 15);
    R2(block, d, e, a, b, c, 0);
    R2(block, c, d, e, a, b, 1);
    R2(block, b, c, d, e, a, 2);
    R2(block, a, b, c, d, e, 3);
    R2(block, e, a, b, c, d, 4);
    R2(block, d, e, a, b, c, 5);
    R2(block, c, d, e, a, b, 6);
    R2(block, b, c, d, e, a, 7);
    R3(block, a, b, c, d, e, 8);
    R3(block, e, a, b, c, d, 9);
    R3(block, d, e, a, b, c, 10);
    R3(block, c, d, e, a, b, 11);
    R3(block, b, c, d, e, a, 12);
    R3(block, a, b, c, d, e, 13);
    R3(block, e, a, b, c, d, 14);
    R3(block, d, e, a, b, c, 15);
    R3(block, c, d, e, a, b, 0);
    R3(block, b, c, d, e, a, 1);
    R3(block, a, b, c, d, e, 2);
    R3(block, e, a, b, c, d, 3);
    R3(block, d, e, a, b, c, 4);
    R3(block, c, d, e, a, b, 5);
    R3(block, b, c, d, e, a, 6);
    R3(block, a, b, c, d, e, 7);
    R3(block, e, a, b, c, d, 8);
    R3(block, d, e, a, b, c, 9);
    R3(block, c, d, e, a, b, 10);
    R3(block, b, c, d, e, a, 11);
    R4(block, a, b, c, d, e, 12);
    R4(block, e, a, b, c, d, 13);
    R4(block, d, e, a, b, c, 14);
    R4(block, c, d, e, a, b, 15);
    R4(block, b, c, d, e, a, 0);
    R4(block, a, b, c, d, e, 1);
    R4(block, e, a, b, c, d, 2);
    R4(block, d, e, a, b, c, 3);
    R4(block, c, d, e, a, b, 4);
    R4(block, b, c, d, e, a, 5);
    R4(block, a, b, c, d, e, 6);
    R4(block, e, a, b, c, d, 7);
    R4(block, d, e, a, b, c, 8);
    R4(block, c, d, e, a, b, 9);
    R4(block, b, c, d, e, a, 10);
    R4(block, a, b, c, d, e, 11);
    R4(block, e, a, b, c, d, 12);
    R4(block, d, e, a, b, c, 13);
    R4(block, c, d, e, a, b, 14);
    R4(block, b, c, d, e, a, 15);

    /* Add the working vars back into digest[] */
    digest[0] += a;
    digest[1] += b;
    digest[2] += c;
    digest[3] += d;
    digest[4] += e;

    /* Count the number of transformations */
    transforms++;
}

inline static void buffer_to_block(
    const std::string& buffer, u32 block[BLOCK_INTS]
) {
    /* Convert the std::string (byte buffer) to a u32 array (MSB) */
    for (size_t i = 0; i < BLOCK_INTS; i++) {
        block[i] = (buffer[4 * i + 3] & 0xff) | (buffer[4 * i + 2] & 0xff) << 8
            | (buffer[4 * i + 1] & 0xff) << 16
            | (buffer[4 * i + 0] & 0xff) << 24;
    }
}

inline SHA1::SHA1() { reset(digest, buffer, transforms); }

inline void SHA1::update(const std::string& s) {
    std::istringstream is(s);
    update(is);
}

inline void SHA1::update(std::istream& is) {
    while (true) {
        char sbuf[BLOCK_BYTES];
        is.read(sbuf, BLOCK_BYTES - buffer.size());
        buffer.append(sbuf, (std::size_t)is.gcount());
        if (buffer.size() != BLOCK_BYTES) {
            return;
        }
        u32 block[BLOCK_INTS];
        buffer_to_block(buffer, block);
        transform(digest, block, transforms);
        buffer.clear();
    }
}

/*
 * Add padding and return the message digest.
 */

inline std::array<u8, 20> SHA1::final() {
    /* Total number of hashed bits */
    u64 total_bits = (transforms * BLOCK_BYTES + buffer.size()) * 8;

    /* Padding */
    buffer += (char)0x80;
    size_t orig_size = buffer.size();
    while (buffer.size() < BLOCK_BYTES) {
        buffer += (char)0x00;
    }

    u32 block[BLOCK_INTS];
    buffer_to_block(buffer, block);

    if (orig_size > BLOCK_BYTES - 8) {
        transform(digest, block, transforms);
        for (size_t i = 0; i < BLOCK_INTS - 2; i++) {
            block[i] = 0;
        }
    }

    /* Append total_bits, split this u64 into two u32 */
    block[BLOCK_INTS - 1] = (u32)total_bits;
    block[BLOCK_INTS - 2] = (u32)(total_bits >> 32);
    transform(digest, block, transforms);

    auto result = std::array<u8, 20>{
        static_cast<u8>(digest[0] >> 24), static_cast<u8>(digest[0] >> 16),
        static_cast<u8>(digest[0] >> 8),  static_cast<u8>(digest[0] >> 0),
        static_cast<u8>(digest[1] >> 24), static_cast<u8>(digest[1] >> 16),
        static_cast<u8>(digest[1] >> 8),  static_cast<u8>(digest[1] >> 0),
        static_cast<u8>(digest[2] >> 24), static_cast<u8>(digest[2] >> 16),
        static_cast<u8>(digest[2] >> 8),  static_cast<u8>(digest[2] >> 0),
        static_cast<u8>(digest[3] >> 24), static_cast<u8>(digest[3] >> 16),
        static_cast<u8>(digest[3] >> 8),  static_cast<u8>(digest[3] >> 0),
        static_cast<u8>(digest[4] >> 24), static_cast<u8>(digest[4] >> 16),
        static_cast<u8>(digest[4] >> 8),  static_cast<u8>(digest[4] >> 0),
    };

    /* Reset for next run */
    reset(digest, buffer, transforms);

    return result;
}

#endif /* SHA1_HPP */