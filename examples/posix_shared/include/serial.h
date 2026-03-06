/**
 * @file: posix_serial.h                                                                           *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-16                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_EXAMPLE_TEST_POSIX_SERIAL_H
#define MT_UHF_SDK_EXAMPLE_TEST_POSIX_SERIAL_H

//C
#include <string.h>

//Metratec SDK
#include <metratec/uhf_reader/public/typedef.h>

/**
 * @brief               Open a serial connect if possible
 * 
 * @param port_name     Name of the connection as string (e.g. "/dev/ttyACM0")
 * @param rx_time       The receiver wait time in ms (0 means instant return of read)
 * 
 * @return              0 on success
 * @returns             else POSIX error code
 */
int comm_start(char *port_name, size_t rx_time);

/**
 * @brief               A function usable for / in polling callback
 * @details             Will get data from serial (if possible) and
 *                      push them into the SDK. 
 *                      Type equals mt_uhf_poll_cb_t to allow direct use as poll_cb
 * 
 * @return              Error code in sdk format
 */
mt_uhf_errorcode_t comm_update(void);

/**
 * @brief               Close the port (if open)
 */
void comm_stop(void);

#endif //include guard