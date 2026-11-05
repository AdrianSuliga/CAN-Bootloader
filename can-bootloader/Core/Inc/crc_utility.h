#ifndef __CRC_UTILITY_H
#define __CRC_UTILITY_H

#include "stm32f7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * To calculate a CRC32 checksum of data, initialize the checksum with CRC_Checksum_Initialize,
 * feed it all the data through CRC_Checksum_Update, then after the final update calculate the final
 * checksum with CRC_Checksum_Finalize.
 *
 * Note: CRC_Checksum_Update works on full words, not bytes. This means that the size parameter MUST
 * be divisible by 4, otherwise the calculation will not work.
 */
void CRC_Checksum_Initialize();
void CRC_Checksum_Update(const uint8_t *data, uint32_t size);
uint32_t CRC_Checksum_Finalize();

#ifdef __cplusplus
}
#endif

#endif /* __CRC_UTILITY_H */
