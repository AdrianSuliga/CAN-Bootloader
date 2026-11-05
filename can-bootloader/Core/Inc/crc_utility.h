#ifndef __CRC_UTILITY_H
#define __CRC_UTILITY_H

#include "stm32f7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * To calculate a CRC32 checksum of data, initialize the checksum with CRC_INITIAL_VALUE,
 * feed it all the data through CRC_Checksum_Update, then after the final update calculate the final
 * checksum with CRC_Checksum_Finalize.
 * */
#define CRC_INITIAL_VALUE 0xFFFFFFFFu
uint32_t CRC_Checksum_Update(uint32_t crc, const uint8_t data[], uint32_t size);
uint32_t CRC_Checksum_Finalize(uint32_t crc);

#ifdef __cplusplus
}
#endif

#endif /* __CRC_UTILITY_H */
