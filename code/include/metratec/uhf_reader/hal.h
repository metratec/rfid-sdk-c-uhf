/**
 * @file: hal.h                                                                                    *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-30                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_HAL_H
#define MT_UHF_SDK_HAL_H

//Includes config (as types depend in it) and used types including the relevant C headers
#include <metratec/uhf_reader/public/typedef.h>

/**
 * @defgroup HAL Hardware Abstraction Layer
 * Functions needed by the library to communicate with the reader over a character based interface
 * and or process the data. 
 * @{
 */

/**
 * @brief           Function which should be filled with the characters received from the Reader 
 *                  Will copy the received data to an internal buffer and is save to be called from interrupts
 * @attention       Needs to be implemented by the user!
 * 
 * @param[in] data  Pointer to buffer of containing the received characters
 * @param data_len  Number of received characters
 * 
 * @return          Number of characters which were copied into the internal buffer, max at data_len
 * @returns         Error codes if something fails
 */
mt_uhf_errorcode_t mt_rfid_reader_rx(void *data, size_t data_len);

/**
 * @brief           Returns difference of size to fill level of the internal rx buffer
 * @attention       Needs to be implemented by the user!
 * 
 * @return          Number of empty bytes in buffer
 */
size_t mt_rfid_reader_rx_remaining_empty(void);

/**
 * @brief       Function called by the API to get the current time
 *              Should return a monotonically rising time in ms, e.g uptime or the lower 32bits of a system time
 * @attention   Needs to be implemented by the user!
 * 
 * @return      current time in ms.
 */
uint32_t mt_rfid_reader_get_time(void);

/**
 * @brief           Function called by the API to send characters to the reader
 *                  Needs to be implemented by the user! Should take the characters in data and 
 *                  copy them into a tx buffer or send them immediately to the reader
 * @attention       Needs to be implemented by the user!
 * 
 * @param[in] data  Pointer to memory location where the characters to be sent are located, 
 *                  might be invalid directly after returning, so data needs to be copied for safety!
 * @param data_len  Number of characters to be sent 
 * 
 * @return          Number of characters copied into the tx buffer
 * @returns         Error codes if something fails
 */
mt_uhf_errorcode_t mt_rfid_reader_tx(const uint8_t *data, size_t data_len);

/**
 * @brief           Waits for the given time, up to user implementation
 * @attention       Needs to be implemented by the user!
 * 
 * @param time_ms   Time to wait in milliseconds
 */
extern void mt_cmd_wait(uint32_t time_ms);

#if MT_UHF_USE_ASSERT
/**
 * @brief           This function is used by the lib to print out assertions.
 * @attention       Needs to be implemented by the user if MT_UHF_USE_ASSERT is set!
 *
 * @param message   The assertion message to print
 */
void mt_rfid_reader_assert_log(char *message);
#endif

/**@}*/

#endif //include guard