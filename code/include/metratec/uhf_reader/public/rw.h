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
 * @brief           Reading data from a membank of a tag (found by TID or, if missing, by EPC)
 * @param data      Buffer where the data read is writting to
 * @param data_len  The amount of data to read (up to 16, needs to be even)
 * @param offset    The address inside the membank (needs to be even)
 * @param bank      The bank to read from (PC, EPC, USR, TID, RES), instead of TID maybe use mt_uhf_inventory_read_tid()
 * @param tag       The tag to look for (will msk on TID if exists and check EPC, too)
 * @return          POSIX Error Code
 *                  returns  0 if successful, 
 *                  Negative values for errors
 */
int mt_uhf_read_data(enum mt_uhf_mem_bank      bank,
                     unsigned                  len,
                     unsigned                  offset,
                     struct mt_uhf_buffer     *answer_buffer,
                     struct mt_uhf_buffer_epc *epc_buffer,
                     unsigned                  answer_buffer_count);

/**
 * @brief           Writing data to a membank of a tag (found by TID or, if missing, by EPC)
 * @param data      Buffer of data to write to the tag
 * @param data_len  Amount of data to write (up to 16, needs to be even)
 * @param offset    The address inside the membank (needs to be even)
 * @param bank      The bank to write to (PC, EPC, USR, TID, RES), instead of RES write use password write , especially if only one is needed
 * @param tag       The tag to look for (will msk on TID if exists and check EPC, too)
 * @return          POSIX Error Code
 */
int mt_uhf_write_data(enum mt_uhf_mem_bank      bank,
                      unsigned                  data_len,
                      unsigned                  offset,
                      uint8_t                  *data,
                      struct mt_uhf_buffer_epc *epc_buffer,
                      unsigned                  buffer_count);

//TODO: Add lock, unlock, permalock, kill