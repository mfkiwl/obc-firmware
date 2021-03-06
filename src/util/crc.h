/**
 * @defgroup crc
 *
 * @brief CRC Routines
 *
 * @details 
 *
 * @ingroup util
 */

#ifndef __UTIL_CRC_H__
#define __UTIL_CRC_H__

#include "../firmware.h"

/**
 * @function
 * @ingroup crc
 *
 * Calculate the 8-bit CRC of the input buffer,
 * using the CRC-8 standard.
 *
 * @param buf Input buffer to calculated
 * @param size Size of the input buffer (in bytes)
 * @returns The 8bit CRC of the input buffer
 */
uint8_t Util_crc8(const uint8_t *buf, uint32_t size);

/**
 * @function
 * @ingroup crc
 *
 * Calculate the 16-bit CRC of the input buffer,
 * using the CRC-16-CCITT standard.
 *
 * @param buf Input buffer to calculated
 * @param size Size of the input buffer (in bytes)
 * @returns The 16bit CRC of the input buffer
 */
uint16_t Util_crc16(const uint8_t *buf, uint32_t size);

/**
 * @function
 * @ingroup crc
 *
 * Calculate the 32-bit CRC of the input buffer,
 * using the CRC-32 standard.
 *
 * @param buf Input buffer to calculated
 * @param size Size of the input buffer (in bytes)
 * @returns The 32bit CRC of the input buffer
 */
uint32_t Util_crc32(const uint8_t *buf, uint32_t size);

#endif /* __UTIL_CRC_H__ */