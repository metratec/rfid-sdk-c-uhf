/**
 * @file: parse.h                                                                                  *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-30                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#pragma once

/**
 * @brief           Parses ASCII data into an integer
 * @param data      The ASCII data given
 * @param data_len  The number of bytes in data
 * @param result    A pointer where the result should be saved to
 *                  If NULL the function can still check if data is an int
 *                  representation
 * @return          0 if successful, *result will contain the result (if not NULL)
 *                  -EBADMSG if data contains invalid characters or only sign char
 *                  -EINVAL if data is NULL or data_len is zero
 *                  -ERANGE if the value doesn't fit into an int
 */
int mt_parse_int(const char *data, size_t data_len, int *result);

/**
 * @brief               User available function to parse ASCII data with hex coded data
 * @param hex_array     ASCII data, only allows capital letters and numbers (0-9, A-F)
 * @param hex_len       Length of data, hex_len must be even 
 * @param check         Only if true the data is checked to be hex formated, 
 *                      if false the byte_array will contain errors due to wrong input
 * @param byte_array    A target buffer for parsed data
 * @param byte_size     Size of the target buffer, needs to be at least half the size of hex_len
 * @return              0 if successful
 *                      -EINVAL if hex_array or byte_array are missing (and hex_len is not zero)
 *                      -EINVAL also if hex_len is not even
 *                      -ENOBUFS if byte_size is not big enough
 *                      -EBADMSG if check is enabled and hex_array does contain non hex chars
 */
int mt_parse_hex_array_to_bytes(const char *hex_array,
                                size_t      hex_len,
                                bool        check,
                                uint8_t    *byte_array,
                                size_t      byte_size);

/**
 * @brief           Check if ASCII data is hex coded data
 * @param data      ASCII bytes, allowed to be NULL if data_len is zero
 * @param data_len  Amount of bytes in data, allowed to be zero
 * @return          true if all bytes are hex coded (only capitalized)
 *                  true also if there are no bytes (data_len is zero)
 *                  false if any character is not in 0-9, A-F
 */
bool mt_parse_check_hex(const char *data, unsigned data_len);

/**
 * @brief           Prints 2 times data_len ASCII chars to target
 * @param data      The data to print as hex
 * @param data_len  Amount of data in bytes
 * @param target    A buffer to put the data to. Must have 2*data_len bytes available
 */
void mt_print_hex(const uint8_t *data, size_t data_len, char *target);
