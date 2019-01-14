/**
 * File: ciropkt.h
 * 
 * Description: Ciropkt functions and definitions.
 * 
 * Serialized packet : |address|command|----data----|crc|
 * 
 * Author: pepemanboy
 */

//// INCLUDES

#include "result.h"
#include "cobs.h"
#include "crc8.h"
#include <string.h>

#ifndef CIROPKT_H
#define CIROPKT_H

#ifdef __cplusplus
extern "C" {
#endif

//// DEFINES

/** Default static initialization value of packet object */
#define pkt_INIT { 0, 0, 0, 0, 0}

/** Declare and initialize a packet variable */
#define pkt_VAR(p) packet_t p = pkt_INIT

/** Maximum data in packet */
#define pkt_MAXDATASIZE 10

/** Header size: address and command */
#define pkt_HEADERSIZE (2 * sizeof(uint8_t))

/** Crc size: 1 byte */
#define pkt_CRCSIZE sizeof(uint8_t)

/** Extra size in packet besides data */
#define pkt_EXTRASIZE (pkt_HEADERSIZE + pkt_CRCSIZE)

/** Calculate total unserialized packet bytes for given payload data size */
#define pkt_LENGTH(d) (pkt_HEADERSIZE + (d) + pkt_CRCSIZE)

/** Max unserialized space that a packet can take */
#define pkt_MAXSPACE pkt_LENGTH(pkt_MAXDATASIZE)

//// STRUCTURES DEFINITIONS

/** C ciropkt packet object */
typedef struct packet_t packet_t;
struct packet_t
{
    uint8_t address; ///< Slave address
    uint8_t command; ///< Packet command
    uint8_t crc; ///< Calculated/received crc

    size_t data_size; ///< Packet used data bytes
    
    uint8_t data[pkt_MAXDATASIZE]; ///< Internal data buffer    
};

//// FORWARD FUNCTIONS DECLARATIONS

/** Update object data and data_size */
res_t pktUpdate(packet_t *p, const void *dat, const size_t len);

/** Check if pkt has valid CRC */
bool pktCheck(const packet_t *p);

/** Refresh packet CRC */
res_t pktRefresh(packet_t *p);

/** Serialize packet */
res_t pktSerialize(const packet_t *p, uint8_t *buf, size_t *len);

/** Deserialize packet */
res_t pktDeserialize(packet_t *p, const uint8_t *buf, const size_t len);

#ifdef __cplusplus
}
#endif

#endif // CIROPKT_H


