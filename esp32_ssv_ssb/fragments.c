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


#include "fragments.h"


static u32_t seq;
static u32_t *buf_seq;
static u8_t frag_num;
static u8_t buf[FRAG_MAX_SIZE_ADV_DATA];
static u8_t *cur_payload;
static size_t cur_payload_len;
static frag_t cur_frag;
static u8_t lastFrag;

static u8_t hmac[FRAG_SIZE_HMAC];
static u8_t key[] = "SuperSecretKey16";

void _add_hmac(){
  int err;
  u16_t vendor = 0x0059; // Manufacture ID
  u8_t prototype = 0x40;
  blake2s_state b2s_state;
  err = blake2s_init_key(&b2s_state, FRAG_SIZE_HMAC, key, strlen(key));
  err = blake2s_update(&b2s_state, (u8_t*)&vendor, FRAG_SIZE_VENDOR);
  err = blake2s_update(&b2s_state, &prototype, FRAG_SIZE_PROTOTYPE);
  err = blake2s_update(&b2s_state, (u8_t*)&seq, FRAG_SIZE_SEQENZ);
  err = blake2s_update(&b2s_state, cur_payload, cur_payload_len);
  err = blake2s_final(&b2s_state, hmac, FRAG_SIZE_HMAC);
  memcpy(cur_payload+cur_payload_len, hmac, FRAG_SIZE_HMAC);
  cur_payload_len += FRAG_SIZE_HMAC;
}

frag_t * frag_init(u32_t start_seq){

  seq = start_seq;
  cur_frag = (frag_t){.data=buf, .len=0};

  u16_t *vendor = (u16_t*) (buf + 0);
  u8_t *prototype = (u8_t*) (buf + FRAG_SIZE_VENDOR);
  buf_seq = (u32_t*) (buf + FRAG_SIZE_HEADER);
  lastFrag = 1;
  frag_num = 0x00;

  *vendor = 0x0059; // Manufacture ID
  *prototype = 0x40; // Protokoll ID

  return &cur_frag;
}

int frag_setpayload(u8_t *payload, size_t len){
  if(len > ((FRAG_MAX_AMOUNT*FRAG_MAX_SIZE_DATA)-FRAG_SIZE_HMAC)){
    return -1; // Error: payload to big -> bufferoverflow
  }
  if(!lastFrag){
    return -2; // Error: last payload not completly retrieved yet
  }
  seq++;
  lastFrag = 0x00;
  frag_num = 0;
  cur_payload = payload;
  cur_payload_len = len;
  _add_hmac();
  return 0;
}

frag_t * frag_getfragment(){

  if(lastFrag){
    return NULL;
  }

  size_t idx = frag_num*FRAG_MAX_SIZE_DATA; // index in payload
  size_t len = cur_payload_len-idx; // length of payload part
  if(len > FRAG_MAX_SIZE_DATA){
    len = FRAG_MAX_SIZE_DATA; // length of payload part
    lastFrag = 0x00;
  }
  else{
    lastFrag = 0x01;
  }
  memcpy(buf+FRAG_SIZE_HEADER+FRAG_SIZE_SEQENZ, cur_payload+idx, len);
  len += FRAG_SIZE_HEADER + FRAG_SIZE_SEQENZ; // length of fragment

  *buf_seq = (seq << 5)+(frag_num); // 5 Bits for lastFrag_bit and frag_num
  if(lastFrag){
   *buf_seq += (1<<4);
  }

  frag_num++;

  cur_frag.len = len; // length of fragment
  return &cur_frag;
}

u32_t get_frag_seq(){
  return seq;
}
