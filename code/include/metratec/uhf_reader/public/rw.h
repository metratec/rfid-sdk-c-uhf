/**
 * @file: rw.h                                                                                     *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-23                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#pragma once

//basics, including other basic includes
#include "typedef.h"

/**
 * @brief                       Read data from tag
 * @param bank                  Bank to read the data from (usually mt_uhf_mem_bank_USR)
 * @param len                   Amount of data to read in bytes (up to 16)
 * @param offset                Start address in bank to read from (needs to be even)
 * @param answer_buffer_data    Pointer to one or multiple buffers to put the data
 * @param answer_buffer_count   Pointer to one or multiple buffers to put the epc of found data to (optional)
 * @param epc_buffer            Pointer to one EPC to use as a mask in READ command (up to 16 bytes will be used)
 * @param error_count           Buffer to get number of times a tag reported with an error, only valid with return >= 0, optional
 * 
 * @return                      >= 0: Number of successful reads (and therefore number of data available), < 0: POSIX Error Code
 */
int mt_uhf_read_data(enum mt_uhf_mem_bank      bank,
                     unsigned                  len,
                     unsigned                  offset,
                     struct mt_uhf_buffer     *answer_buffer_data,
                     struct mt_uhf_buffer_epc *answer_buffer_epc,
                     unsigned                  answer_buffer_count,
                     struct mt_uhf_buffer_epc *mask,
                     unsigned                 *error_count);

/**
 * @brief                       Writing data to a membank of a tag (found by EPC mask if available)
 * @param bank                  The bank to write to (PC, EPC, USR, TID, RES), instead of RES write use password write , especially if only one is needed
 * @param data                  Buffer of data to write to the tag
 * @param data_len              Amount of data to write in bytes (up to 16)
 * @param offset                The address inside the membank (needs to be even)
 * @param answer_buffer_epc     Pointer to one or multiple buffers to put the epc of written tag to (optional)
 * @param answer_buffer_count   The number of EPC buffers available
 * @param mask                  The EPC of the tag to look for (max 12 bytes are used), optional
 * @param error_count           Buffer to get number of times a tag reported with an error, only valid with return >= 0, optional
 * 
 * @return                      >= 0: Number of successful writes, < 0: POSIX Error Code
 */
int mt_uhf_write_data(enum mt_uhf_mem_bank      bank,
                      uint8_t                  *data,
                      unsigned                  data_len,
                      uint32_t                  offset,
                      struct mt_uhf_buffer_epc *answer_buffer_epc,
                      unsigned                  answer_buffer_count,
                      struct mt_uhf_buffer_epc *mask,
                      unsigned                 *error_count);

//TODO: Add lock, unlock, permalock, kill
