/**
 * @file: config.h                                                                                 *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_CONFIG_H
#define MT_UHF_SDK_CONFIG_H

#if MT_UHF_MUX_MAX
#define MAX_ANTENNAS_MUXED ((MT_UHF_MUX_MAX) * (MT_UHF_ANTENNA_COUNT))
#else
#define MAX_ANTENNAS_MUXED (MT_UHF_ANTENNA_COUNT)
#endif

//Buffer taking mt_rfid_reader_rx() data, one byte of them won't be used so only one byte less can be handled
#ifndef MT_UHF_SETTING_RXBUFFER_SIZE
#define MT_UHF_SETTING_RXBUFFER_SIZE 1024
#endif

//library assert is defaulting to disabled
#ifndef MT_UHF_USE_ASSERT
#define MT_UHF_USE_ASSERT 0
#endif

/*Number of bytes available for event types (+HBT, +CINV etc), that's data 
received without a command*/
#define MT_UHF_MAX_EVENT_TYPES 10

// Buffer size for a single answer line
#define MT_UHF_PARSER_BUF_SIZE 256
#if MT_UHF_PARSER_BUF_SIZE >= MT_UHF_SETTING_RXBUFFER_SIZE
#warning Parser buffer can't be fully used as receive buffer isn't as big
#endif

// Used for debug: if true uses printf to printout send and received frames
#ifndef DEBUG_PRINTOUT_PRINTF
#define DEBUG_PRINTOUT_PRINTF false
#endif

// Some devices have an issue that causes them to send an additional frame start
// This setting instructs the frame checking to ignore the seconds frame start
// If disabled the checker function will return an error
#define IGNORE_DOUBLE_FRAME_START true

#endif //include guard