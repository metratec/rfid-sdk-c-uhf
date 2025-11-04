/**
 * @file: inventory.h                                                                              *
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

#include "typedef.h"

/**
 * @brief           Single base inventory
 * @param cb        A callback function to call when a tag is received. 
 *                  Must return true if the tag should also be buffered (so can be used as a filter)
 *                  Can be NULL to have no callback
 * @param buffer    A buffer to put the found tags into. Can be NULL to not buffer at all
 * @return          POSIX Error Code,
 *                  Returns 0 if a function is done successfully (OK),
 *                  -EFAULT if a function is done with an ERROR,
 *                  below zero as error (most common: -EBADMSG for all types of format and framing and value errors)
 */
int mt_uhf_inventory(mt_uhf_tag_cb cb, struct mt_uhf_inventory_buffer *buffer, uint32_t timeout_ms);

/**
 * @brief               Use the reader's AT+TID command to get the TID of all tags (faster and less mem than with INV setting with TID)
 * @param TIDs          Put found TIDs here
 * @param tid_len       The max number if TIDs
 * @param timeout_ms    Timeout in milliseconds, use zero for default
 * @return              POSIX Error Code (if negative) OR 
 *                      Number of tags, it can be more than tid_len, in this case TIDs conains the tid_len first TIDs
 */
int mt_uhf_inventory_read_tid(struct mt_uhf_buffer_tid TID[],
                              uint32_t                 tid_len,
                              uint32_t                 timeout_ms);

/**
 * @brief               Run a single multiplexed inventory (see mt_uhf_set_multiplex_antennas())
 * @param cb            The callback to get the tags and round finish notification (can be NULL)
 * @param buffer        A buffer to put the found tags in (can be NULL)
 * @param timeout_ms    The timeout in milliseconds (can be 0 for default but that is likely too short)
 * @return              POSIX Error Code
 */
int mt_uhf_inventory_automux(mt_uhf_tag_cb                   cb,
                             struct mt_uhf_inventory_buffer *buffer,
                             uint32_t                        timeout_ms);

/**
 * @brief               Single reported inventory (runs for a given time and counts the tags, identified by EPC)
 * @param cb            A callback function to call when a tag is received. 
 *                      Must return true if the tag should also be buffered (so can be used as a filter)
 *                      The tag info will contain the count
 *                      Can be NULL to have no callback
 * @param buffer        A buffer to put the found tags into. Can be NULL to not buffer at all
 * @param run_time_ms   The time how long the inv runs in milliseconds. Can be up to 1000.
 * @param timeout_ms    Timeout, needs to be at least 100ms + run_time_ms
 * @return              POSIX Error Code,
 *                      Returns 0 if done successfully (OK),
 *                      -EFAULT if done with an ERROR,
 *                      below zero as error (most common: -EBADMSG for all types of format and framing and value errors)
 */
int mt_uhf_inventory_reported(mt_uhf_tag_cb                   cb,
                              struct mt_uhf_inventory_buffer *buffer,
                              uint32_t                        run_time_ms,
                              uint32_t                        timeout_ms);

/**
 * @brief           Will stop any running continious inventory
 * @param  timeout  Time for the answer. Keep in mind the current inventory round will be finished first so this may take a while
 * @return          POSIX Error Code,
 *                  Returns 0 if done successfully (OK),
 *                  -EFAULT if done with an ERROR,
 *                  -EINVAL if no inventory is running
 *                  below zero as error (most common: -EBADMSG for all types of format and framing and value errors)
 */
int mt_uhf_inventory_stop(uint32_t timeout_ms);

/**
 * @brief                   Continious reported inventory (runs for a given time and counts the tags, identified by EPC)
 * @param cb                A callback function to call when a tag is received. Gets called with NULL parameter when a round is done
 *                          Must return true if the tag should also be buffered (so can be used as a filter)
 *                          The tag info will contain the count. 
 *                          cb can be NULL to have no callback
 * @param buffer            A buffer to put the found tags into. Can be NULL to not buffer at all
 * @param round_done_cb     Callback to know when a round is done (and the buffer should be handled)
 * @param time_ms           Run time of a single report round (up to 1000 ms)
 * @return                  POSIX Error Code,
 *                          Returns 0 if done successfully (OK),
 *                          -EFAULT if done with an ERROR,
 *                          -EBADMSG for all types of format and framing and value errors
 */
int mt_uhf_inventory_reported_start_continious(
    mt_uhf_tag_cb                   cb,
    struct mt_uhf_inventory_buffer *buffer,
    void(round_done_cb)(struct mt_uhf_inventory_buffer *inv_buf),
    uint32_t time_ms);

/**
 * @brief                   Start continious inventory
 *                          Call mt_uhf_resolve() to parse the events reguarly (or have any other
 *                          command running)
 * @param cb                Callback for tags found (can be NULL)
 * @param buffer            Pointer for a buffer to put tag data to
 * @param round_done_cb     A callback to call when a round is done, can be NULL, mostly needed
 *                          to correctly handle and reset the buffer
 * @return                  POSIX Error Code
 */
int mt_uhf_inventory_start_continious(mt_uhf_tag_cb                   cb,
                                      struct mt_uhf_inventory_buffer *buffer,
                                      void(round_done_cb)(struct mt_uhf_inventory_buffer *inv_buf));

/**
 * @brief                   Start continious multiplexed inventory
 *                          Call mt_uhf_resolve() to parse the events reguarly (or have any other
 *                          command running)
 * @param cb                Callback for tags found (can be NULL)
 * @param buffer            Pointer for a buffer to put tag data to
 * @param round_done_cb     A callback to call when a round is done, can be NULL, mostly needed
 *                          to correctly handle and reset the buffer
 * @return                  POSIX Error Code
 */
int mt_uhf_inventory_automux_start_continious(
    mt_uhf_tag_cb                   cb,
    struct mt_uhf_inventory_buffer *buffer,
    void(round_done_cb)(struct mt_uhf_inventory_buffer *inv_buf));