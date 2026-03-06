/**
 * @file: api.h                                                                                    *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-23                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 *                                                                                                 *
 * @brief Main include file for functions provided by the UHF reader
 */

#ifndef MT_UHF_SDK_PUBLIC_API_H
#define MT_UHF_SDK_PUBLIC_API_H

/**
 * @brief  file for functions to usually use
 * 
 * @details Reader settings can be found here while the tag functions
 * can be found in inventory.h and rw.h (already included by this file)
 * 
 * All needed types also get included (from typedef.h)
 * 
 * This includes NO headers from foldern "intern"
 * If any fuctions are needed non blocking they need to implemented by the user.
 * 
 * Alternatively use the blocking_cb setting and get your actions needed into the cb
 */

//basics, including other basic includes
#include <metratec/uhf_reader/public/typedef.h>

//rfid commands seperated for readability
#include <metratec/uhf_reader/public/inventory.h>
#include <metratec/uhf_reader/public/rw.h>

/**
 * @brief                       Initialize (Reset and init the reader) and set up the SDK for use
 * 
 * @param unexpected_frame_cb   Callback for whenever a frame can't be parsed into a
 *                              regular type like frame start, data etc.
 * @param polling_cb            Callback called by the SDK to get more RX data (if set)
 * @param blocking_cb           Called by the SDK when in a blocking function and it can't do anything
 * @param reset_tried_cb        Called after a reset got triggered, this may need to handle 
 *                              reconnecting USB or waiting for boot up.
 *      @warning                Call of reset_tried_cb is no guarantee that the reset 
 *                              command was successful!
 * 
 * @return                      mt_uhf_errorcode_success (0) on success
 * @returns                     mt_uhf_errorcode_no_valid_device if the device's firmware revision
 *                              is below minimum firmware revision
 * @returns                     < 0: Error code
 * @note                        Errors can be caused by 2 main sources:
 *                              - Wrong setup (config, memory, invalid parameter)
 *                              - Failed device setup (device not running, invalid firmware version, 
 *                                command execution failed, communication error...)
 */
mt_uhf_errorcode_t mt_uhf_init(mt_uhf_unexpected_frame_cb_t unexpected_frame_cb,
                               mt_uhf_poll_cb_t             polling_cb,
                               mt_uhf_blocking_cb_t         blocking_cb,
                               mt_uhf_errorcode_t (*reset_tried_cb)(void));

/**
 * @brief               Sets the (default) timeouts used by the library
 * 
 * @param timeout       Default timeout used for most commands
 * @param timeout_cinv  Default timeout used in continuous commands like CINV, CMINV ...
 */
void mt_uhf_timeout_set(uint32_t timeout, uint32_t timeout_cinv);

/**
 * @brief   Reset the receiver state so after an error the next answer won't get rejected
 */
void mt_uhf_rx_reset(void);

/**
 * @brief       Call this in a frame handling function 
 * @details     For the user this is only the "unexpected_frame_cb" given to 
 *              mt_uhf_init() if the user decides to manually handle the frame)
 * 
 * @return      Always returns mt_uhf_errorcode_already so the handler can return 
 *              directly from the function
 */
mt_uhf_errorcode_t mt_uhf_current_frame_handled(void);

/**
 * @brief       Get the current state of the reader info.
 * @details     If it's already known won't ask the reader but take the existing data
 * 
 * @param id    Pointer to put the data to
 * 
 * @return      mt_uhf_errorcode_success (0) on success
 * @returns     mt_uhf_errorcode_no_valid_device if the device's firmware revision
 *              is below minimum firmware revision
 * @returns     < 0: Other error code
 */
mt_uhf_errorcode_t mt_uhf_get_identification(struct mt_uhf_reader_identification *id);

/**
 * @brief       Start or disable the heartbeat event
 * @warning     Call mt_uhf_resolve() to parse the events regularly (or have any other
 *              command running)
 * 
 * @param time  Time in seconds, must be 0-60, zero equals disable
 * @param cb    The callback called whenever the reader gives a heartbeat. Should be NULL if disabling
 * 
 * @return      mt_uhf_errorcode_success (0) on success
 * @returns     < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_heartbeat_set(uint8_t time, mt_uhf_data_cb_t cb);

/**
 * @brief                   Sets the output power per Antenna Port, trying to configure non-existing Ports will abort the command
 * 
 * @param[in] power         pointer to an array of power values, one per Antenna to be configured
 * @param num_of_antennas   Number of antennas to set and therefore also the size of power. Allows also 0, implying to set for all antennas and power having one element
 * 
 * @return                  mt_uhf_errorcode_success (0) on success
 * @returns                 < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_power(const uint8_t *power, size_t num_of_antennas);

/**
 * @brief                   Get the power level of the currently active antennas
 * 
 * @param power             Pointer to put the values to
 * @param num_of_antennas   Size of buffer in power
 * 
 * @return                  >=0: Number of antennas / power values
 * @returns                 < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_power(uint8_t *power, size_t num_of_antennas);

/**
 * @brief           Sets Q Values for anticollision
 * 
 * @param q_initial Initial Q value, should be ~3 for short range Readers and ~5 for long range readers
 * @param q_min     Minimum Q value for dynamic anticollision algorithm
 * @param q_max     Maximum Q value for dynamic anticollision algorithm
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_q_value(uint8_t q_initial, uint8_t q_min, uint8_t q_max);

/**
 * @brief           Set the RF mode together with the region so it is checked to be a valid pair
 * 
 * @param region    The region to use (only one so mt_uhf_region_all is not valid)
 * @param rf_mode   A mode from the enum values
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         mt_uhf_errorcode_not_supported if the device doesn't support the region
 *                  or the mode is not supported in this regional setting
 * @returns         < 0: Other error code
 */
mt_uhf_errorcode_t mt_uhf_set_rf_mode(enum mt_uhf_region region, enum mt_uhf_rf_mode rf_mode);

/**
 * @brief           Get the region / mode value mirroring mt_uhf_set_rf_mode()
 * 
 * @param  region   Pointer for region value
 * @param  rf_mode  Pointer for RF mode value
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_rf_mode(enum mt_uhf_region *region, enum mt_uhf_rf_mode *rf_mode);

/**
 * @brief           Sets targeted session for QUERY
 * 
 * @param session   see GS1-EPC-Gen2v2 
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_session(enum mt_uhf_session session);

/**
 * @brief           Gets targeted session for QUERY mirroring mt_uhf_set_session()
 * 
 * @param session   pointer to write the value to
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_session(enum mt_uhf_session *session);

/**
 * @brief           Configures the antenna for inventory (not muxed / reported) and other tag commands (read, write, lock...)
 * 
 * @param antenna   Antenna identifier, starting from 1
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_antenna(uint8_t antenna);

/**
 * @brief           Get the currently set antenna value 
 * 
 * @param antenna   Pointer to write value to
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_antenna(uint8_t *antenna);

/**
 * @brief               Set the muxing list for muxed / reported inventory
 * @warning             Beware: The mux list allows antenna values up to the 
 *                      theoretical maximal amount even if there aren't enough
 *                      antennas available (external mux not properly set)
 * 
 * @param mux_list      An array of antenna values
 * @param mux_list_len  Mux length. If mux_list is != NULL this is the length of mux_list
 *                      If mux_list is NULL it is the amount of antennas to mux
 *                      starting with 1 (so NULL, 2 is idential in muxing to {1,2}, 2)
 *                      This allows setting a one element list without having to declare
 *                      memory.
 * 
 * @return              mt_uhf_errorcode_success (0) on success
 * @returns             < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_multiplex_antennas(const uint8_t *mux_list, size_t mux_list_len);

/**
 * @brief               Get the current muxing list for muxed / reported inventory
 * 
 * @param mux_list      Memory to put the data to
 * @param mux_list_len  Maximum available memory (lack of memory will cause an error)
 * 
 * @return              > 0: Success, number of entries in list
 *                      Beware: A number of one means that the muxing will be done from 1 to (return value)
 *                      So return value of 1 and mux_list[0] == 2 equals the behaviour of
 *                      return value 2 and mux_list[0] == 1 and mux_list[1] == 2
 * @returns             < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_multiplex_antennas(uint8_t *mux_list, size_t mux_list_len);

/**
 * @brief           Sets the amount of external multiplexer antennas for all (internal) antennas
 * @details         Each value will replace 1 antenna with the given amount of antennas, apart from
 *                  0, which will disable muxing for this antenna.
 *                  This will impact muxed inventory use
 * 
 * @param mux_list  An array of values counting antennas
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_external_multiplexer(uint8_t mux_list[MT_UHF_ANTENNA_COUNT]);

/**
 * @brief           Get the current amount of multiplexers on each antenna
 * 
 * @param mux_list  Pointer to a memory to put the current states to
                    Needs to provide MT_UHF_ANTENNA_COUNT elements to fill
 * 
 * @return          > 0: Amount of entries in the mux_list
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_external_multiplexer(uint8_t *mux_list);

/**
 * @brief                   Configure settings for all following inventory calls
 * 
 * @param only_new_tags     Each Tag will only answer once until it leaves the field
 * @param show_rssi         Append the measured RSSI value to each Inventory entry
 * @param read_tid          Automatically read and append the TID for each tag during inventory
 * @param fast_start        Skip the reset of all Tags into State A for faster Queries, needed for only_new_tags
 *  @attention              Warning: Already powered Tags might not appear during Inventories
 * @param show_phase        Makes the reader give the phase in inventory
 * @param selected          Set which tags should be inventoried (selected, not selected or both)
 * @param target            Set the tags that should answer session based (state A or B)
 * @param rssi_threshold    Give the reader a threshold to show a tag, only used if 
 *                          the device supports the setting
 * 
 * @return                  mt_uhf_errorcode_success (0) on success
 * @returns                 < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_inventory_settings(bool                                 only_new_tags,
                                                 bool                                 show_rssi,
                                                 bool                                 read_tid,
                                                 bool                                 fast_start,
                                                 bool                                 show_phase,
                                                 enum mt_uhf_gen2_inventory_selecting selected,
                                                 enum mt_uhf_gen2_inventory_target    target,
                                                 int rssi_threshold);

/**                         
 * @brief                   Get the current setting mirroring mt_uhf_set_inventory_settings()
 * 
 * @param only_new_tags     Pointer to put the value, can be NULL
 * @param show_rssi         Pointer to put the value, can be NULL
 * @param read_tid          Pointer to put the value, can be NULL
 * @param fast_start        Pointer to put the value, can be NULL
 * @param show_phase        Pointer to put the value, can be NULL
 * @param selected          Pointer to put the value, can be NULL
 * @param target            Pointer to put the value, can be NULL
 * @param rssi_threshold    Pointer to put the value, can be NULL, should be NULL if
 *                          the device doesn't support it
 * 
 * @return                  mt_uhf_errorcode_success (0) on success
 * @returns                 < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_inventory_settings(bool *only_new_tags,
                                                 bool *show_rssi,
                                                 bool *read_tid,
                                                 bool *fast_start,
                                                 bool *show_phase,
                                                 enum mt_uhf_gen2_inventory_selecting *selected,
                                                 enum mt_uhf_gen2_inventory_target    *target,
                                                 int *rssi_threshold);

/**
 * @brief                   Set a mask used for inventory, should only find tags fitting the mask
 * @note                    The command uses the AT+BMSK command only, not AT+MSK
 * 
 * @param target            The bank to apply the mask to (usually that's EPC or TID)
 * @param mask_start_bit    Start address to compare in the membank
 * @param mask              Containing the data to mask with
 * @param mask_len_bit      The length of the mask. The lib needs the rounded up 8th of this value as mask
 * 
 * @return                  mt_uhf_errorcode_success (0) on success
 * @returns                 < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_inventory_mask(enum mt_uhf_mem_bank target,
                                             uint32_t             mask_start_bit,
                                             const uint8_t       *mask,
                                             uint32_t             mask_len_bit);

/**
 * @brief                   Get the current inventory mask, mirroring mt_uhf_set_inventory_mask()
 * 
 * @param target            Pointer to put the value, not allowed to be NULL
 * @param mask_start_bit    Pointer to put the value, can be NULL if mask is NULL
 * @param mask              Pointer to put the value, can be NULL
 * @param mask_buffer_size  Length of mask, should be sizeable enough to take any expected mask
 * @param mask_len_bit      Pointer to put the value, can be zero if mask is NULL
 * 
 * @return                  mt_uhf_errorcode_success (0) on success
 * @returns                 < 0: Error code
 * @returns                 mt_uhf_errorcode_no_buffer: mask was not able to take all the data
 */
mt_uhf_errorcode_t mt_uhf_get_inventory_mask(enum mt_uhf_mem_bank *target,
                                             uint32_t             *mask_start_bit,
                                             uint8_t              *mask,
                                             size_t                mask_buffer_size,
                                             uint32_t             *mask_len_bit);

/**
 * @brief           Set the channel mask bitwise
 * 
 * @param mask      Technically would work with multiple regions,
 *                  but currently only supports ETSI lower band
 *                  because of restrictions. Max value therefor is 0x0F (4 channels set)
 *                  Expects zero for all channels
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_channel_mask(uint64_t mask);

/**
 * @brief           Get the current channel mask, mirroring mt_uhf_set_channel_mask()
 * 
 * @param mask      Pointer to put the value, like in mt_uhf_set_channel_mask() the value
 *                  will be set to zero if all channels are set (masking disabled)
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_channel_mask(uint64_t *mask);

#if MT_UHF_GPI_COUNT
/**
 * @brief           Get the state of all inputs
 * 
 * @param buffer    Target buffer for the values
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_input(mt_uhf_input_states_t buffer);
#endif

#if MT_UHF_GPO_COUNT
/**
 * @brief           Set the state of all outputs
 * 
 * @param buffer    The value to set the outputs to.
 *  @note           You can use mt_uhf_io_state_no_change value
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_set_output(mt_uhf_output_states_t buffer);
#endif

/**
 * @brief           Function which needs to be called regularly to parse event data while no command is running
 * @details         Needed to parse continuous inventories, heartbeats or input events.
 *                  Checks the internal character buffer and updates the internal statemachine, calling callbacks if required.
 *                  Shouldn't be called from an interrupt context! 
 *                  This function isn't required to be called if no events are active.
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         mt_uhf_errorcode_busy_call_again  if current command handling on current data hasn't finished yet
 * @returns         mt_uhf_errorcode_no_data_available if the current data isn't enough for more handling
 * @returns         mt_uhf_errorcode_general_fault on device error
 * @returns         mt_uhf_errorcode_format_fault, mt_uhf_errorcode_inconsistent, mt_uhf_errorcode_range on communication errors
 * @returns         < 0: Other error codes
 */
mt_uhf_errorcode_t mt_uhf_resolve(void);

//TODO: Add FDB, PLY, SUS, HOT, IEV, IP4, PMACT, SRACT commands, impinj_custom_settings

#endif //include guard