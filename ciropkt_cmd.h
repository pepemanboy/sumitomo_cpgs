/**
 * File: ciropkt_cmd.h
 * 
 * Description: Ciropkt commands and packets.
 * 
 * Author: pepemanboy
 */

//// INCLUDES

#include <stdint.h>

#ifndef CIROPKT_CMD_H
#define CIROPKT_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

/** Commands */
typedef enum
{
  cmd_CPGInfo,
}cmds_e;

/** CPG info structure */
typedef struct CPGInfo CPGInfo;
struct CPGInfo
{
  uint8_t cpg_id;
};

#ifdef __cplusplus
}
#endif

#endif // CIROPKT_CMD_H


