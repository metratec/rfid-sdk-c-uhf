/**
 * @file: framehandler.h                                                                           *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-23                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#pragma once

#include "receive_buffer.h"

enum uhfv2_frametype {
    //needed if data is completely handled and the "current" was moved to "last"
    uhfv2_frametype_none = 0,
    uhfv2_frametype_invalid,

    uhfv2_frametype_answer_start, //consists of an empty data element with a frameend
    uhfv2_frametype_echo, //constist of one data element with frameend, data is identical to last_cmd
    uhfv2_frametype_answer_finish_success, //consists of an ok element with a frameend
    uhfv2_frametype_answer_finish_failed,  //consists of an error element with a frameend
    uhfv2_frametype_data_element, //constists of one data element without a frameend, can be part of answer or event
    uhfv2_frametype_data_finish, //constists of one data element with a frameend, can be part of answer or event
};
struct uhfv2_frame {
    enum uhfv2_frametype type;
    char                *data;
};

/**
 * @brief               Function to handle a frame
 * @param data          The frame data
 * @param data_len      Length of frame data (bytes)
 * @return              0 if the frame was successfully handled
 *                      -return value of unknown_frame_cb
 *                      -BADMSG if the frame was faulty in any way
 *                      -EALREADY if the frame is done
 *                      -other negatives from sub functions
 */
int mt_uhf_framehandler(char *data, size_t data_len);

/**
 * @brief           Searches for a frame callback by the prefix of the data element
 * @param prefix    A string containing the prefix of the data
 * @return          Pointer to the fitting element containing the needed callback
 *                  NULL of there is no fitting CB in the lookup data (or prefix is NULL)
 */
struct mt_uhf_frame_data_cb_lookup *mt_uhf_framehandler_get_data_cb(const char *prefix);

/**
 * @brief           Add a callback to the lookup table for mt_uhf_framehandler_get_data_cb()
 *                  If the prefix already exists it is overwritten
 * @param prefix    The prefix that should trigger the callback
 * @param cb        The callback to call (NULL to remove the entry)
 * @return          0 if successful (maybe added or removed or already empty)
 *                  -NOBUFS if lookup table is already full
 *                  -EINVAL if there is no prefix
 */
int mt_uhf_framehandler_set_data_cb(const char *prefix, mt_uhf_data_cb_t cb);