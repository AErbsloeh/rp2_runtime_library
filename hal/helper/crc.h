#ifndef HAL_HELPER_CRC_H_
#define HAL_HELPER_CRC_H_


#include <stdint.h>
#include <stddef.h>


/*! \brief Compute CRC-16-CCITT for a byte buffer.
 *  \param data  Byte buffer to checksum
 *  \param len   Number of bytes in the buffer
 *  \return      16-bit CRC value
 */
uint16_t crc16_ccitt(const uint8_t *data, size_t len);

#endif // HAL_CRC_CRC_H_
