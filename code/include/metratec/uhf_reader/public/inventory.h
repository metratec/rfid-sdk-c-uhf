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

#ifndef MT_UHF_SDK_PUBLIC_INVENTORY_H
#define MT_UHF_SDK_PUBLIC_INVENTORY_H

#include <metratec/uhf_reader/public/typedef.h>

/**
 * @brief           Single base inventory
 * 
 * @param cb        A callback function to call when a tag is received. 
 *                  Must return true if the tag should also be buffered (so can be used as a filter)
 *                  Can be NULL to have no callback
 * @param buffer    A buffer to put the found tags into. Can be NULL to not buffer at all
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         mt_uhf_errorcode_general_fault if device answered with ERROR
 * @returns         other error codes for different communication or device errors
 */
mt_uhf_errorcode_t mt_uhf_inventory(mt_uhf_tag_cb                   cb,
                                    struct mt_uhf_inventory_buffer *buffer,
                                    uint32_t                        timeout_ms);

/**
 * @brief               Use the reader's AT+TID command to get the TID of all tags (faster and less mem than with INV setting with TID)
 * 
 * @param TID           Put found TIDs here
 * @param tid_len       The max number of TIDs
 * @param timeout_ms    Timeout in milliseconds, use zero for default
 * 
 * @return              >= 0: Number of tags found (can be more than tid_len; in that case TID contains the first tid_len entries)
 * @returns             mt_uhf_errorcode_general_fault if device answered with ERROR
 * @returns             other error codes for different communication or device errors
 *                      
 */
mt_uhf_errorcode_t mt_uhf_inventory_read_tid(struct mt_uhf_buffer_tid *TID,
                                             uint32_t                  tid_len,
                                             uint32_t                  timeout_ms);

/**
 * @brief               Run a single multiplexed inventory (see mt_uhf_set_multiplex_antennas())
 * 
 * @param cb            The callback to get the tags and round finish notification (can be NULL)
 * @param buffer        A buffer to put the found tags in (can be NULL)
 * @param timeout_ms    The timeout in milliseconds (can be 0 for default but that is likely too short)
 * 
 * @return              mt_uhf_errorcode_success if function is done with OK
 * @returns             mt_uhf_errorcode_general_fault if device answered with ERROR
 * @returns             other error codes for different communication or device errors
 */
mt_uhf_errorcode_t mt_uhf_inventory_automux(mt_uhf_tag_cb                   cb,
                                            struct mt_uhf_inventory_buffer *buffer,
                                            uint32_t                        timeout_ms);

/**
 * @brief               Single reported inventory (runs for a given time and counts the tags, identified by EPC)
 * 
 * @param cb            A callback function to call when a tag is received. 
 *                      Must return true if the tag should also be buffered (so can be used as a filter)
 *                      The tag info will contain the count
 *                      Can be NULL to have no callback
 * @param buffer        A buffer to put the found tags into. Can be NULL to not buffer at all
 * @param run_time_ms   The time how long the inv runs in milliseconds. Can be up to 1000.
 * @param timeout_ms    Timeout, needs to be at least 100ms + run_time_ms
 * 
 * @return              mt_uhf_errorcode_success if function is done with OK
 * @returns             mt_uhf_errorcode_general_fault if device answered with ERROR
 * @returns             other error codes for different communication or device errors
 */
mt_uhf_errorcode_t mt_uhf_inventory_reported(mt_uhf_tag_cb                   cb,
                                             struct mt_uhf_inventory_buffer *buffer,
                                             uint32_t                        run_time_ms,
                                             uint32_t                        timeout_ms);

/**
 * @brief               Will stop any running continuous inventory
 * 
 * @param timeout_ms    Time for the answer. Keep in mind the current inventory round will be finished first so this may take a while
 * 
 * @return              mt_uhf_errorcode_success if function is done with OK
 * @returns             mt_uhf_errorcode_general_fault if device answered with ERROR
 * @returns             other error codes for different communication or device errors
 */
mt_uhf_errorcode_t mt_uhf_inventory_stop(uint32_t timeout_ms);

/**
 * @brief                   Continuous reported inventory (runs for a given time and counts the tags, identified by EPC)
 * 
 * @param cb                A callback function to call when a tag is received. Gets called with NULL parameter when a round is done
 *                          Must return true if the tag should also be buffered (so can be used as a filter)
 *                          The tag info will contain the count. 
 *                          cb can be NULL to have no callback
 * @param buffer            A buffer to put the found tags into. Can be NULL to not buffer at all
 * @param round_done_cb     A callback to call when a round is done, can be NULL, mostly needed
 *                          to correctly handle and reset the buffer
 * @param run_time_ms       Run time of a single report round (up to 1000 ms)
 * 
 * @return                  mt_uhf_errorcode_success if function is done with OK
 * @returns                 mt_uhf_errorcode_general_fault if device answered with ERROR
 * @returns                 other error codes for different communication or device errors
 */
mt_uhf_errorcode_t mt_uhf_inventory_reported_start_continious(mt_uhf_tag_cb cb,
                                                              struct mt_uhf_inventory_buffer *buffer,
                                                              cinv_round_done_cb_t round_done_cb,
                                                              uint32_t             time_ms);

/**
 * @brief                   Start continuous inventory
 * @attention               Call mt_uhf_resolve() to parse the events regularly (or have any other
 *                          command running)
 * 
 * @param cb                Callback for tags found (can be NULL)
 * @param buffer            A buffer to put the found tags into. Can be NULL to not buffer at all
 * @param round_done_cb     A callback to call when a round is done, can be NULL, mostly needed
 *                          to correctly handle and reset the buffer
 * 
 * @return                  mt_uhf_errorcode_success if function is done with OK
 * @returns                 mt_uhf_errorcode_general_fault if device answered with ERROR
 * @returns                 other error codes for different communication or device errors
 */
mt_uhf_errorcode_t mt_uhf_inventory_start_continious(mt_uhf_tag_cb                   cb,
                                                     struct mt_uhf_inventory_buffer *buffer,
                                                     cinv_round_done_cb_t            round_done_cb);

/**
 * @brief                   Start continuous multiplexed inventory
 * @attention               Call mt_uhf_resolve() to parse the events regularly (or have any other
 *                          command running)
 * 
 * @param cb                Callback for tags found (can be NULL)
 * @param buffer            Pointer for a buffer to put tag data to
 * @param round_done_cb     A callback to call when a round is done, can be NULL, mostly needed
 *                          to correctly handle and reset the buffer
 * 
 * @return                  mt_uhf_errorcode_success if function is done with OK
 * @returns                 mt_uhf_errorcode_general_fault if device answered with ERROR
 * @returns                 other error codes for different communication or device errors
 */
mt_uhf_errorcode_t mt_uhf_inventory_automux_start_continious(mt_uhf_tag_cb                   cb,
                                                             struct mt_uhf_inventory_buffer *buffer,
                                                             cinv_round_done_cb_t round_done_cb);

#endif //include guard