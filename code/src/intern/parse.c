/**
 * @file: parse.c                                                                                  *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-30                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#include <metratec/uhf_reader/intern/common.h>

mt_uhf_errorcode_t mt_parse_int(const char *data, size_t data_len, int *result)
{
    if (!data || data_len == 0)
        return mt_uhf_errorcode_invalid_parameter;
    const char *parse = data;
    if (data[0] == '-' || data[0] == '+') {
        data_len--;
        if (data_len == 0)
            return mt_uhf_errorcode_range;
        parse++;
    }
    bool negative = (data[0] == '-');
    int  value    = 0;
    for (size_t i = 0; i < data_len; i++) {
        uint8_t v = parse[i] - '0';
        if (v > 9)
            return mt_uhf_errorcode_range; //Invalid Character
        if (negative) {
            int min_value = (INT_MIN + v) / 10;
            if (value < min_value)
                return mt_uhf_errorcode_range;
            value = value * 10 - v;
        } else {
            int max_value = (INT_MAX - v) / 10;
            if (value > max_value)
                return mt_uhf_errorcode_range;
            value = value * 10 + v;
        }
    }
    if (result)
        *result = value;
    return mt_uhf_errorcode_success;
}

/**
 * @brief       Internal function to parse a hex char to binary value
 * @param hex   A hex ASCII char, only allows capital letters and numbers (0-9, A-F)
 * @return      Returns the value, invalid values are only handled by assert!
 *              Can return valid values (<= 0xF) from invalid character!
 *              Careful and internal use only
 *              mt_parse_check_hex() can help to precheck data before parsing
 */
static inline uint8_t _hex_char_2_bin(const char hex)
{
    uint8_t v = hex - ('0' - 0x00);
    if (v <= 9)
        return v;
    v = hex - ('A' - 0x0A);
    mt_rfid_reader_assert(v >= 0x0A && v <= 0x0F, "No hex char");
    return v;
}

mt_uhf_errorcode_t mt_parse_hex_array_to_bytes(const char *hex_array,
                                               size_t      hex_len,
                                               bool        check,
                                               uint8_t    *byte_array,
                                               size_t      byte_size)
{
    if (hex_len == 0)
        return mt_uhf_errorcode_success;
    if (!hex_array || hex_len & 1 || !byte_array)
        return mt_uhf_errorcode_invalid_parameter;
    if (byte_size < hex_len / 2)
        return mt_uhf_errorcode_invalid_parameter;
    if (check && !mt_parse_check_hex(hex_array, hex_len))
        return mt_uhf_errorcode_range; //Invalid Character

    uint8_t *target = byte_array;
    for (size_t i = 0; i < hex_len; i += 2) {
        uint8_t high_nibble = _hex_char_2_bin(hex_array[i]);
        uint8_t low_nibble  = _hex_char_2_bin(hex_array[i + 1]);
        *target             = (high_nibble * 16) + low_nibble;
        target++;
    }
    return mt_uhf_errorcode_success;
}

bool mt_parse_check_hex(const char *data, unsigned data_len)
{
    mt_rfid_reader_assert(data || !data_len, "Datalength with no data to check");
    for (unsigned i = 0; i < data_len; i++)
        switch (data[i]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            continue;
        default:
            return false;
        }
    return true;
}

static void _print_nibble(uint8_t n, char *t)
{
    *t = (n <= 9) ? n + '0' : (n - 0xA + 'A');
}
void mt_print_hex(const uint8_t *data, size_t data_len, char *target)
{
    for (size_t done = 0; done < data_len; done++) {
        _print_nibble(data[done] >> 4, target++);
        _print_nibble(data[done] & 0xF, target++);
    }
}