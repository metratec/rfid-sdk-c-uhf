/**
 * @file: posix_interface.h                                                                        *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-16                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_EXAMPLE_TEST_POSIX_INTERFACE_H
#define MT_UHF_SDK_EXAMPLE_TEST_POSIX_INTERFACE_H

//C
#include <stdbool.h>

// Tools
#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

/**
 * @brief   Initialize the interface
 * @details Random and termination signal handler for example
 * 
 * @return  0 on success
 * @returns -1 on fail
 */
int interface_init(void);

/**
 * @brief   Gives information if a termination request was catched
 * @details If true the caller shall make a program stop
 * 
 * @return  True if requested, else false. State is not reset
 */
bool abort_requested(void);

#endif //include guard