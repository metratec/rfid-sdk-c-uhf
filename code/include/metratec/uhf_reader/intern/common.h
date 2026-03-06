/**
 * @file: common.h                                                                                 *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-16                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_INTERN_COMMON_H
#define MT_UHF_SDK_INTERN_COMMON_H

//For include abstraction in intern

//First hal, including typedef, this includes config and type defining C headers
#include <metratec/uhf_reader/hal.h>

//Then implementations
#include <metratec/uhf_reader/intern/assert.h>
#include <metratec/uhf_reader/intern/framehandler.h>
#include <metratec/uhf_reader/intern/parse.h>
#include <metratec/uhf_reader/intern/reader.h>
#include <metratec/uhf_reader/intern/receive_buffer.h>

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#endif //include guard