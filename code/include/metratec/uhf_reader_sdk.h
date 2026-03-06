/**
 * @file: uhf_reader_sdk.h                                                                         *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-23                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 * 
 * @brief: Main entry point, including is enough for most most use cases
 */

/** @defgroup Entrypoint
 * Interface file including all headers needed by the user of the library.
 * - Settings in config.h may be changed by the user
 * - Device selection includes device settings depending on the selected device
 * - HAL contains declarations of all functions needed by the library to work properly. 
 *   They beed to be implemented by the user.
 * - API contains the functions to use to interact with the reader
 */

#ifndef MT_UHF_SDK_BASE_H
#define MT_UHF_SDK_BASE_H

#ifdef __cplusplus
#warning Compiled with C++ but C is expected
#endif

#include <stddef.h>
#include <stdint.h>

#include <metratec/uhf_reader/public/errorcodes.h>

//Get settings (used in library and hal)
#include <metratec/uhf_reader/config.h>

//Add HAL functions (used in library)
#include <metratec/uhf_reader/hal.h>

//Add Library functions
#include <metratec/uhf_reader/public/api.h>

#endif //include guard