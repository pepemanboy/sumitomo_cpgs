/**
 * File: result.h
 * 
 * Description: Error result codes.
 * 
 * Author: pepemanboy
 */

//// INCLUDES

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef RESULT_H
#define RESULT_H

/** Result type */
typedef uint8_t res_t;

/** Error codes enum */
typedef enum
{
  Ok = 0,
  Error,
  EMemory,  
  EData,
  EPartial,
  EParse,
  EFormat,
  EAddress,
  ECommand,
}error_e;

#endif // RESULT_H
