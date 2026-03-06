/**
 * @file: tag.h                                                                               *
 * Project: metratec-reader-sdk-c                                                                  *
 * Created Date: 2024-01-10                                                                        *
 * Author: Nils Harder                                                                             *
 * -----                                                                                           *
 * Copyright (C) 2024                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 *                                                                                                 *
 * @brief Provided constants for tag data
 */

#ifndef MT_UHF_SDK_PUBLIC_TAG_H
#define MT_UHF_SDK_PUBLIC_TAG_H

/** @brief The EPC length is coded in words (16 bit sized) in a 5 bit value ranging from 0 to 31 */
#define MT_UHF_GEN2_MAX_EPC_BYTES 62

/** @brief      The maximum length of a TID
 *  @details    https://ref.gs1.org/standards/tds/ page 222 state
 *              TID extension content up to address 0xCF (bitwise) which makes it 0xD0 bits long,
 *              but not including the settable serial number length which CAN be 48 bits (that 
 *              are included in the 0xD0) but can also be extended by up to 6 * 16 bits. 
 *  @note       This should add up to (0xD0 + 0x60) / 8 = 26 + 12 = 38 bytes
 * */
#define MT_UHF_GEN2_MAX_TID_BYTES ((0xD0 + 6 * 16) / 8)

/**@}*/

#endif //include guard