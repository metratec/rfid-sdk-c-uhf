/**
 * @file: errorcodes.h                                                                             *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-05-05                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 * 
 * @brief Declaration of errorcodes as C has no guaranty of existing error codes
 */

#ifndef MT_UHF_SDK_PUBLIC_ERRORCODES_H
#define MT_UHF_SDK_PUBLIC_ERRORCODES_H

#include <limits.h>
#include <stdint.h>
/**
 * @brief   A custom error code type based on int
 * @details Allows positive values as success to directly have a return value
 *          mt_uhf_errorcode_t is compatible to int
 *          mt_uhf_error2string() allows to get a string description for printouts
 */
typedef enum mt_uhf_errorcode {
    mt_uhf_errorcode_success           = 0,
    mt_uhf_errorcode_success_max       = INT_MAX,
    mt_uhf_errorcode_general_fault     = -1,
    mt_uhf_errorcode_busy_call_again   = -10,
    mt_uhf_errorcode_invalid_parameter = -11, ///One parameter or a set is out of range
    mt_uhf_errorcode_memory_full       = -12, ///Existing buffer is full
    mt_uhf_errorcode_no_buffer         = -13, ///Pointer needed is zero
    mt_uhf_errorcode_not_available     = -14, ///Not supported in this state (but is in general)
    mt_uhf_errorcode_not_supported     = -15, ///Not supported (by this device)
    mt_uhf_errorcode_no_valid_device   = -16, ///Firmware not matching requirements or no init
    mt_uhf_errorcode_no_data_available = -20, ///can often be 0 instead
    mt_uhf_errorcode_timeout           = -21, ///Can't complete in time
    mt_uhf_errorcode_already           = -22, ///Data already set to requested value
    mt_uhf_errorcode_format_fault      = -30, ///Parsing failed to invalid format
    mt_uhf_errorcode_range             = -31, ///Received data is out of range (length or value)
    mt_uhf_errorcode_inconsistent      = -32, ///Inconstistent answer
} mt_uhf_errorcode_t;

/**
 * @brief               Provides a string decode of error code
 * 
 * @param errorcode     Any error code, unknowns will still get a string saying it's unknown
 * 
 * @return              String
 */
const char *mt_uhf_error2string(mt_uhf_errorcode_t errorcode);

#endif //include guard