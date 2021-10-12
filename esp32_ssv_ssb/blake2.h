/*
   BLAKE2 reference source code package - optimized C implementations

   Written in 2012 by Samuel Neves <sneves@dei.uc.pt>
   Modified in 2021 by Tim Rambousky <tra@ssv-embedded.de>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/


#ifndef __BLAKE2_H__
#define __BLAKE2_H__

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
    #define BLAKE2_DLL_IMPORT __declspec(dllimport)
    #define BLAKE2_DLL_EXPORT __declspec(dllexport)
    #define BLAKE2_DLL_PRIVATE
#elif __GNUC__ >= 4
  #define BLAKE2_DLL_IMPORT   __attribute__ ((visibility ("default")))
  #define BLAKE2_DLL_EXPORT   __attribute__ ((visibility ("default")))
  #define BLAKE2_DLL_PRIVATE  __attribute__ ((visibility ("hidden")))
#else
  #define BLAKE2_DLL_IMPORT
  #define BLAKE2_DLL_EXPORT
  #define BLAKE2_DLL_PRIVATE
#endif

#if defined(BLAKE2_DLL)
  #if defined(BLAKE2_DLL_EXPORTS) // defined if we are building the DLL
    #define BLAKE2_API BLAKE2_DLL_EXPORT
  #else
    #define BLAKE2_API BLAKE2_DLL_IMPORT
  #endif
  #define BLAKE2_PRIVATE BLAKE2_DLL_PRIVATE // must only be used by hidden logic
#else
  #define BLAKE2_API
  #define BLAKE2_PRIVATE
#endif

#if defined(__cplusplus)
extern "C" {
#elif defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif

  enum blake2s_constant
  {
    BLAKE2S_BLOCKBYTES = 64,
    BLAKE2S_OUTBYTES   = 32,
    BLAKE2S_KEYBYTES   = 32,
    BLAKE2S_SALTBYTES  = 8,
    BLAKE2S_PERSONALBYTES = 8
  };

  enum blake2b_constant
  {
    BLAKE2B_BLOCKBYTES = 128,
    BLAKE2B_OUTBYTES   = 64,
    BLAKE2B_KEYBYTES   = 64,
    BLAKE2B_SALTBYTES  = 16,
    BLAKE2B_PERSONALBYTES = 16
  };

#pragma pack(push, 1)
  typedef struct __blake2s_param
  {
    uint8_t  digest_length; // 1
    uint8_t  key_length;    // 2
    uint8_t  fanout;        // 3
    uint8_t  depth;         // 4
    uint32_t leaf_length;   // 8
    uint8_t  node_offset[6];// 14
    uint8_t  node_depth;    // 15
    uint8_t  inner_length;  // 16
    // uint8_t  reserved[0];
    uint8_t  salt[BLAKE2S_SALTBYTES]; // 24
    uint8_t  personal[BLAKE2S_PERSONALBYTES];  // 32
  } blake2s_param;

  typedef struct __blake2s_state
  {
    uint32_t h[8];
    uint32_t t[2];
    uint32_t f[2];
    uint8_t  buf[2 * BLAKE2S_BLOCKBYTES];
    uint32_t buflen;
    uint8_t  outlen;
    uint8_t  last_node;
  } blake2s_state;

  typedef struct __blake2b_param
  {
    uint8_t  digest_length; // 1
    uint8_t  key_length;    // 2
    uint8_t  fanout;        // 3
    uint8_t  depth;         // 4
    uint32_t leaf_length;   // 8
    uint64_t node_offset;   // 16
    uint8_t  node_depth;    // 17
    uint8_t  inner_length;  // 18
    uint8_t  reserved[14];  // 32
    uint8_t  salt[BLAKE2B_SALTBYTES]; // 48
    uint8_t  personal[BLAKE2B_PERSONALBYTES];  // 64
  } blake2b_param;

  typedef struct __blake2b_state
  {
    uint64_t h[8];
    uint64_t t[2];
    uint64_t f[2];
    uint8_t  buf[2 * BLAKE2B_BLOCKBYTES];
    uint32_t buflen;
    uint8_t  outlen;
    uint8_t  last_node;
  } blake2b_state;

  typedef struct __blake2sp_state
  {
    blake2s_state S[8][1];
    blake2s_state R[1];
    uint8_t  buf[8 * BLAKE2S_BLOCKBYTES];
    uint32_t buflen;
    uint8_t  outlen;
  } blake2sp_state;

  typedef struct __blake2bp_state
  {
    blake2b_state S[4][1];
    blake2b_state R[1];
    uint8_t  buf[4 * BLAKE2B_BLOCKBYTES];
    uint32_t buflen;
    uint8_t  outlen;
  } blake2bp_state;
#pragma pack(pop)

  // Streaming API
  BLAKE2_API int blake2s_init( blake2s_state *S, size_t outlen );
  BLAKE2_API int blake2s_init_key( blake2s_state *S, size_t outlen, const void *key, size_t keylen );
  BLAKE2_API int blake2s_init_param( blake2s_state *S, const blake2s_param *P );
  BLAKE2_API int blake2s_update( blake2s_state *S, const uint8_t *in, size_t inlen );
  BLAKE2_API int blake2s_final( blake2s_state *S, uint8_t *out, size_t outlen );

  BLAKE2_API int blake2b_init( blake2b_state *S, size_t outlen );
  BLAKE2_API int blake2b_init_key( blake2b_state *S, size_t outlen, const void *key, size_t keylen );
  BLAKE2_API int blake2b_init_param( blake2b_state *S, const blake2b_param *P );
  BLAKE2_API int blake2b_update( blake2b_state *S, const uint8_t *in, size_t inlen );
  BLAKE2_API int blake2b_final( blake2b_state *S, uint8_t *out, size_t outlen );

  BLAKE2_API int blake2sp_init( blake2sp_state *S, size_t outlen );
  BLAKE2_API int blake2sp_init_key( blake2sp_state *S, size_t outlen, const void *key, size_t keylen );
  BLAKE2_API int blake2sp_update( blake2sp_state *S, const uint8_t *in, size_t inlen );
  BLAKE2_API int blake2sp_final( blake2sp_state *S, uint8_t *out, size_t outlen );

  BLAKE2_API int blake2bp_init( blake2bp_state *S, size_t outlen );
  BLAKE2_API int blake2bp_init_key( blake2bp_state *S, size_t outlen, const void *key, size_t keylen );
  BLAKE2_API int blake2bp_update( blake2bp_state *S, const uint8_t *in, size_t inlen );
  BLAKE2_API int blake2bp_final( blake2bp_state *S, uint8_t *out, size_t outlen );

  // Simple API
  BLAKE2_API int blake2s( uint8_t *out, const void *in, const void *key, size_t outlen, size_t inlen, size_t keylen );
  BLAKE2_API int blake2b( uint8_t *out, const void *in, const void *key, size_t outlen, size_t inlen, size_t keylen );

  BLAKE2_API int blake2sp( uint8_t *out, const void *in, const void *key, size_t outlen, size_t inlen, size_t keylen );
  BLAKE2_API int blake2bp( uint8_t *out, const void *in, const void *key, size_t outlen, size_t inlen, size_t keylen );

  static inline int blake2( uint8_t *out, const void *in, const void *key, size_t outlen, size_t inlen, size_t keylen )
  {
    return blake2b( out, in, key, outlen, inlen, keylen );
  }

#if defined(__cplusplus)
}
#endif

#endif

#ifndef __BLAKE2_IMPL_H__
#define __BLAKE2_IMPL_H__

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Add no suffix to the functions */
#define SUFFIX

/* Test for a little-endian machine */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define NATIVE_LITTLE_ENDIAN
#endif

#ifdef __cplusplus
}
#endif
#endif /* CONFIG_H */

#define BLAKE2_IMPL_CAT(x,y) x ## y
#define BLAKE2_IMPL_EVAL(x,y)  BLAKE2_IMPL_CAT(x,y)
#define BLAKE2_IMPL_NAME(fun)  BLAKE2_IMPL_EVAL(fun, SUFFIX)

static inline uint32_t load32( const void *src )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  uint32_t w;
  memcpy( &w, src, sizeof( w ) );
  return w;
#else
  const uint8_t *p = ( uint8_t * )src;
  uint32_t w = *p++;
  w |= ( uint32_t )( *p++ ) <<  8;
  w |= ( uint32_t )( *p++ ) << 16;
  w |= ( uint32_t )( *p++ ) << 24;
  return w;
#endif
}

static inline uint64_t load64( const void *src )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  uint64_t w;
  memcpy( &w, src, sizeof( w ) );
  return w;
#else
  const uint8_t *p = ( uint8_t * )src;
  uint64_t w = *p++;
  w |= ( uint64_t )( *p++ ) <<  8;
  w |= ( uint64_t )( *p++ ) << 16;
  w |= ( uint64_t )( *p++ ) << 24;
  w |= ( uint64_t )( *p++ ) << 32;
  w |= ( uint64_t )( *p++ ) << 40;
  w |= ( uint64_t )( *p++ ) << 48;
  w |= ( uint64_t )( *p++ ) << 56;
  return w;
#endif
}

static inline void store32( void *dst, uint32_t w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  memcpy( dst, &w, sizeof( w ) );
#else
  uint8_t *p = ( uint8_t * )dst;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w;
#endif
}

static inline void store64( void *dst, uint64_t w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  memcpy( dst, &w, sizeof( w ) );
#else
  uint8_t *p = ( uint8_t * )dst;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w;
#endif
}

static inline uint64_t load48( const void *src )
{
  const uint8_t *p = ( const uint8_t * )src;
  uint64_t w = *p++;
  w |= ( uint64_t )( *p++ ) <<  8;
  w |= ( uint64_t )( *p++ ) << 16;
  w |= ( uint64_t )( *p++ ) << 24;
  w |= ( uint64_t )( *p++ ) << 32;
  w |= ( uint64_t )( *p++ ) << 40;
  return w;
}

static inline void store48( void *dst, uint64_t w )
{
  uint8_t *p = ( uint8_t * )dst;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w;
}

static inline uint32_t rotl32( const uint32_t w, const unsigned c )
{
  return ( w << c ) | ( w >> ( 32 - c ) );
}

static inline uint64_t rotl64( const uint64_t w, const unsigned c )
{
  return ( w << c ) | ( w >> ( 64 - c ) );
}

static inline uint32_t rotr32( const uint32_t w, const unsigned c )
{
  return ( w >> c ) | ( w << ( 32 - c ) );
}

static inline uint64_t rotr64( const uint64_t w, const unsigned c )
{
  return ( w >> c ) | ( w << ( 64 - c ) );
}

/* prevents compiler optimizing out memset() */
static inline void secure_zero_memory(void *v, size_t n)
{
#if defined(_WIN32) || defined(WIN32)
  SecureZeroMemory(v, n);
#elif defined(__hpux)
  static void *(*const volatile memset_v)(void *, int, size_t) = &memset;
  memset_v(v, 0, n);
#else
// prioritize first the general C11 call
#if defined(HAVE_MEMSET_S)
  memset_s(v, n, 0, n);
#elif defined(HAVE_EXPLICIT_BZERO)
  explicit_bzero(v, n);
#elif defined(HAVE_EXPLICIT_MEMSET)
  explicit_memset(v, 0, n);
#else
  memset(v, 0, n);
  __asm__ __volatile__("" :: "r"(v) : "memory");
#endif
#endif
}

#endif

