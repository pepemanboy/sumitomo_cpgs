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
  cmd_CPGInitQuery,
  cmd_CPGInfoQuery,
  cmd_CPGInfoReply,
}cmds_e;

/** CPG info structure */
typedef struct CPGInfoReply CPGInfoReply;
struct CPGInfoReply
{
  uint8_t cpg_id;
  uint8_t cpg_count;
  uint8_t cpg_sequence;
};

typedef struct CPGInfoQuery CPGInfoQuery;
struct CPGInfoQuery
{
  uint8_t cpg_sequence;
};

/** CPG info structure */
typedef struct CPGInitQuery CPGInitQuery;
struct CPGInitQuery
{
  uint8_t cpg_channel;
  uint32_t cpg_address;
};

#ifdef __cplusplus
}
#endif

#endif // CIROPKT_CMD_H


