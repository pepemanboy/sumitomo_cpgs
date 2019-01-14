/*
 * File: crc8.c
 * 
 * Description: CRC8 calculation
 * 8-bit with 0x97 polynomial
 * 
 * Author: pepemanboy
 */

//// INCLUDES

#include "ciropkt.h"

//// PUBLIC FUNCTIONS

uint8_t crc8(const void *dat, size_t len) 
{
  uint8_t *d = (uint8_t *)dat;
  uint32_t t;
  uint8_t icrc;
  
  --len;
  icrc = 1;
  t = d[0];
  while (len--)
    t = d[icrc++] ^ _crc_tb0x97[t];
  
  return _crc_tb0x97[t];
}
