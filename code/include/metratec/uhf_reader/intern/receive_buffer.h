/**
 * @file: receive_buffer.h                                                                         *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-23                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_INTERN_RECEIVEBUFFER_H
#define MT_UHF_SDK_INTERN_RECEIVEBUFFER_H

#include <metratec/uhf_reader/intern/common.h>

/**
 * @brief   Set of frame / element delimiter characters
 */
enum mt_uhf_delimiter_char {
    mt_uhf_delimiter_char_frame         = '\n',
    mt_uhf_delimiter_char_frame_element = '\r',
};

/**
 * @brief               Initializes the RX ringbuf, needs a buffer to use
 * 
 * @param buffer        A buffer for the RX ringbuffer to use
 * @param buffer_size   The size of the given buffer. The size should be able to take
 *                      Any possible answer element (+1 byte at least), but better
 *                      more to be able to buffer the incoming data
 * 
 * @return              mt_uhf_errorcode_success
 * @returns             mt_uhf_errorcode_invalid_parameter for no buffer of below 64 bytes size
 */
mt_uhf_errorcode_t at_rx_ring_init(void *buffer, size_t buffer_size);

/**
 * @brief       Pushes data into the RX ring buffer
 * 
 * @param data  The data to push
 * @param len   The amount of data to push if possible
 * 
 * @return      The amount of data actually pushed (can be zero)
 */
size_t at_rx_ring_push(const uint8_t *data, size_t len);

/**
 * @brief       Flushes the ring buffer
 * @details     Useful after communication issue got discovered or if for 
 *              some reason the frame detection gets stuck (should not).
 */
void at_rx_ring_flush(void);

/**
 * @brief                           Get a frame for further work with it
 * 
 * @param target                    Memory to push the frame data to
 * @param max_len                   Size of target memory
 * @param delete_oversized_frames   Delete frames that don't fit into target (danger!)
 * 
 * @return                          >= 0 Frame length including end of frame character(s)
 * @returns                         mt_uhf_errorcode_no_data_available: come back when there's new data
 * @returns                         < mt_uhf_errorcode_success (0): errors
 */

mt_uhf_errorcode_t at_rx_ring_getframe(char *target, size_t max_len, bool delete_oversized_frames);

/**
 * @brief   Gives a bool info if there is new data since the last 
 *          call of this function or of at_rx_ring_flush()
 * 
 * @return  True if there is new data, else false
 */
bool at_rx_new_data(void);

#endif //include guard