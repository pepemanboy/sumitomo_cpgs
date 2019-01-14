/*
 * File: cobs.h
 * 
 * Description: Cobs encoding and decoding functions.
 * Based on Jaques Fortier implementation.
 * 
 * Author: pepemanboy
 */

//// INCLUDES

#include <stdint.h>
#include <stddef.h>

#ifndef COBS_H
#define COBS_H

#ifdef __cplusplus
extern "C" {
#endif

//// FORWARD FUNCTIONS DECLARATIONS

/* Stuffs "length" bytes of data at the location pointed to by
 * "input", writing the output to the location pointed to by
 * "output". Returns the number of bytes written to "output".
 *
 * Remove the "restrict" qualifiers if compiling with a
 * pre-C99 C dialect.
 */
size_t cobs_encode(const uint8_t * input, size_t length, uint8_t * output);

/* Unstuffs "length" bytes of data at the location pointed to by
 * "input", writing the output * to the location pointed to by
 * "output". Returns the number of bytes written to "output" if
 * "input" was successfully unstuffed, and 0 if there was an
 * error unstuffing "input".
 */
size_t cobs_decode(const uint8_t * input, size_t length, uint8_t * output);

#ifdef __cplusplus
}
#endif

#endif // COBS_H

