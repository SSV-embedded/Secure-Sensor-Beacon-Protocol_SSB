/*
    Library for creating Fragments for a BLE Advertising Protocoll (SSV/SSB)
    Copyright (C) <2021> <Tim Rambousky>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
    Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


#ifndef FRAGMENTS_H
#define FRAGMENTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stddef.h>
// make sure the blake lib/files are in your directory
#include "blake2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char       u8_t;
typedef unsigned short      u16_t;
typedef unsigned int        u32_t;

#define FRAG_MAX_AMOUNT 16
#define FRAG_MAX_SIZE_ADV_DATA 26
#define FRAG_SIZE_KEY 16
#define FRAG_SIZE_VENDOR 2
#define FRAG_SIZE_PROTOTYPE 1
#define FRAG_SIZE_HEADER (FRAG_SIZE_VENDOR+FRAG_SIZE_PROTOTYPE)
#define FRAG_SIZE_SEQENZ 4
#define FRAG_SIZE_HMAC 6
#define FRAG_MAX_SIZE_DATA (FRAG_MAX_SIZE_ADV_DATA-FRAG_SIZE_HEADER-FRAG_SIZE_SEQENZ)
#define FRAG_SIZE_PAYLOADBUF (FRAG_MAX_AMOUNT*FRAG_MAX_SIZE_DATA)

typedef struct {
  u8_t *data;
  size_t len;
} frag_t;

frag_t * frag_init(u32_t start_seq);
int frag_setpayload(u8_t *payload, size_t len);
frag_t * frag_getfragment();
u32_t get_frag_seq();

#ifdef __cplusplus
}
#endif

#endif // FRAGMENTS_H
