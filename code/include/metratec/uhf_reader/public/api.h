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
 */

#pragma once

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
#include "typedef.h"

//rfid commands seperated for readability
#include "inventory.h"
#include "rw.h"

/**
 * @brief                   Initialize (Reset and init the reader) and set up the SDK for use
 * @param unknown_frame_cb  unknown_frame_cb for whenever a frame can't be parsed at all
 * @param mt_uhf_poll_cb    Callback called by the SDK to get more RX data (if set)
 * @param blocking_cb       Called by the SDK when in a blocking function and it can't do anything
 * @return                  POSIX Error Code
 */
int mt_uhf_init(int (*unknown_frame_cb)(const char *),
                mt_uhf_poll_cb polling_cb,
                void (*blocking_cb)(void));

/**
 * @brief   Reset the receiver state so after an error the next answer won't get rejected
 * @param   None

 */
void mt_uhf_rx_reset(void);

/**
 * @brief   Call this in a frame handling function (for the user this is only the "unknown_frame_cb" given to mt_uhf_init())
 * @param   None
 * @return  Always returns -EALREADY so the handler can return directly from the function
 */
int mt_uhf_current_frame_handled(void);

/**
 * @brief   Get the current state of the reader info.
 *          If it's not already known tries to get it from reader first
 * @return  Returns NULL if the data is ungetable
 */
struct mt_uhf_reader_identification *mt_uhf_get_identification(void);

/**
 * @brief       Start or disable the heartbeat event
 *              Call mt_uhf_resolve() to parse the events reguarly (or have any other
 *              command running)
 * @param time  Time in seconds, must be 0-60, zero equals disable
 * @param cb    The callback called whenever the reader gives a heartbeat. Should be NULL if disabling
 * @return      POSIX Error Code
 */
int mt_reader_heartbeat_set(uint8_t time, mt_uhf_data_cb_t cb);

/**
 * @brief Sets the output power per Antenna Port, trying to configure non-existing Ports will abort the command
 * @param[in] power pointer to an array of power values, one per Antenna to be configured
 * @param num_of_antennas Size of the given array
 * @return POSIX Error Code, -EINVAL if more Values than configured antennas were provided
 */
int mt_uhf_set_power(const uint8_t *power, size_t num_of_antennas);

/**
 * @brief                   Get the power level of the currently active antennas
 * @param power             Pointer to put the values to
 * @param num_of_antennas   Size of buffer in power
 * @return                  Positive: Number of antennas / power values
 *                          Else: Posix error code
 */
int mt_uhf_get_power(uint8_t *power, size_t num_of_antennas);

/**
 * @brief           Sets Q Values for anticollision
 * @param q_initial Initial Q value, should be ~3 for short range Readers and ~5 for long range readers
 * @param q_min     Minimum Q value for dynamic anticollision algorithm
 * @param q_max     Maximum Q value for dynamic anticollision algorithm
 * @return          POSIX error code
 */
int mt_uhf_set_q_value(uint8_t q_initial, uint8_t q_min, uint8_t q_max);

/**
 * @brief           Set the RF mode together with the region so it is checked to be a valid pair
 * @param region    The region to use (only one so mt_uhf_region_all is not valid)
 * @param rf_mode   A mode from the enum values
 * @return          POSIX error code
 */
int mt_uhf_set_rf_mode(enum mt_uhf_region region, enum mt_uhf_rf_mode rf_mode);

/**
 * @brief           Get the region / mode value mirroring mt_uhf_set_rf_mode()
 * @param  region   Pointer for region value
 * @param  rf_mode  Pointer for RF mode value
 * @return          POSIX error code
 */
int mt_uhf_get_rf_mode(enum mt_uhf_region *region, enum mt_uhf_rf_mode *rf_mode);

#if 0
/**
 * @brief Not implemented
 * @param fast_id 
 * @param tag_focus 
 * @return 
 */
int mt_uhf_set_impinj_custom_settings(bool fast_id, bool tag_focus);
int mt_uhf_get_impinj_custom_settings(bool *fast_id, bool *tag_focus);
//TODO: Integrate a use case
// int mt_uhf_impinj_authentication_service(void);
#endif

/**
 * @brief           Sets targeted session for QUERY
 * @param session   see GS1-EPC-Gen2v2 
 * @return          POSIX Error code
 */
int mt_uhf_set_session(enum mt_uhf_session session);
/**
 * @brief           Gets targeted session for QUERY mirroring mt_uhf_set_session()
 * @param session   pointer to write the value to
 * @return          POSIX Error code
 */
int mt_uhf_get_session(enum mt_uhf_session *session);

/**
 * @brief           Configures the antenna for inventory (not muxed / reported) and other tag commands (read, write, lock...)
 * @param antenna   Antenna identifier, starting from 1
 * @return          POSIX error code, -EINVAL if Antenna is not available
 */
int mt_uhf_set_antenna(uint8_t antenna);

/**
 * @brief           Get the currently set antenna value 
 * @param antenna   Pointer to write value to
 * @return          POSIX error code
 */
int mt_uhf_get_antenna(uint8_t *antenna);

/**
 * @brief               Set the muxing list for muxed / reported inventory
 *                      Beware: The mux list allows antenna values up to the 
 *                      theoretical maximal amount even if there aren't enoth
 *                      antennas available (external mux not properly set)
 * @param mux_list      An array of antenna values
 * @param mux_list_len  The length of the antenna list
 * @return              POSIX error code
 */
int mt_uhf_set_multiplex_antennas(uint8_t *mux_list, size_t mux_list_len);

/**
 * @brief               Get the current muxing list for muxed / reported inventory
 * @param mux_list      Memory to put the data to
 * @param mux_list_len  Maximum available memory (lack of memory will cause an error)
 * @return              POSIX error code
 */
int mt_uhf_get_multiplex_antennas(uint8_t *mux_list, size_t mux_list_len);

/**
 * @brief           Sets the amount of external multiplexer antennas for all (internal) antennas
 *                  Each value will replace 1 antenna with the given amount of antennas, apart from
 *                  0, which will disable muxing for this antenna.
 *                  This will impact muxed inventory use
 * @param mux_list  An array of values counting antennas
 * @return          POSIX error code
 */
int mt_uhf_set_external_multiplexer(uint8_t mux_list[MT_UHF_ANTENNA_COUNT]);

/**
 * @brief           Get the current amount of multiplexers on each antenna
 * @param mux_list  Pointer to a memory to put the current states to
                    Needs to supply MT_UHF_ANTENNA_COUNT elements
 * @return          POSIX error code
 */
int mt_uhf_get_external_multiplexer(uint8_t *mux_list);

/**
 * @brief                   Configure settings for all following inventory calls
 * @param only_new_tags     Each Tag will only answer once until it leaves the field
 * @param show_rssi         Append the measured RSSI value to each Inventory entry
 * @param read_tid          Automatically read and append the TID for each tag during inventory
 * @param fast_start        Skip the reset of all Tags into State A for faster Queries, needed for only_new_tags
 *                          Warning: Already powered Tags might not appear during Inventories
 *                          Use tag selection
 * @param show_phase        Makes the reader give the phase in inventory
 * @param selected          Set which tags should be inventoried (selected, not selected or both)
 * @param target            Set the tags that should answer session based (state A or B)
 * @param rssi_threshold    Give the reader a threshold to show a tag
 * @warning                 Already powered Tags might not appear during Inventories
 * @return                  POSIX Error Code
 */
int mt_uhf_set_inventory_settings(bool                                 only_new_tags,
                                  bool                                 show_rssi,
                                  bool                                 read_tid,
                                  bool                                 fast_start,
                                  bool                                 show_phase,
                                  enum mt_uhf_gen2_inventory_selecting selected,
                                  enum mt_uhf_gen2_inventory_target    target,
                                  int                                  rssi_threshold);

/**                         
 * @brief                   Get the current setting mirroring mt_uhf_set_inventory_settings()
 *                          Pointers can be NULL if the user doen't care about a value
 * @param only_new_tags     Pointer to put the value
 * @param show_rssi         Pointer to put the value
 * @param read_tid          Pointer to put the value
 * @param fast_start        Pointer to put the value
 * @param show_phase        Pointer to put the value
 * @param selected          Pointer to put the value
 * @param target            Pointer to put the value
 * @param rssi_threshold    Pointer to put the value
 * @return                  POSIX Error Code
 */
int mt_uhf_get_inventory_settings(bool                                 *only_new_tags,
                                  bool                                 *show_rssi,
                                  bool                                 *read_tid,
                                  bool                                 *fast_start,
                                  bool                                 *show_phase,
                                  enum mt_uhf_gen2_inventory_selecting *selected,
                                  enum mt_uhf_gen2_inventory_target    *target,
                                  int                                  *rssi_threshold);

/**
 * @brief                   Set a mask used for inventory, should only find tags fitting the mask
 *                          The command uses the AT+BMSK command only, not AT+MSK
 * @param target            The bank to apply the mask to (usually that's EPC or TID)
 * @param mask_start        Start address to compare in the membank
 * @param mask              Containing the data to mask with
 * @param mask_len_bit      The length of the mask. The lib needs the rounded up 8th of this value as mask
 * @return                  POSIX Error Code
 */
int mt_uhf_set_inventory_mask(enum mt_uhf_mem_bank target,
                              uint32_t             mask_start_bit,
                              const uint8_t       *mask,
                              uint32_t             mask_len_bit);

/**
 * @brief                   Get the current inventory mask, mirroring mt_uhf_get_inventory_settings()
 * @param target            Pointer to put the value
 * @param mask_start_bit    Pointer to put the value
 * @param mask              Pointer to put the value
 * @param mask_buffer_size  Pointer to put the value
 * @param mask_len_bit      Pointer to put the value
 * @return                  POSIX Error Code
 */
int mt_uhf_get_inventory_mask(enum mt_uhf_mem_bank *target,
                              uint32_t             *mask_start_bit,
                              uint8_t              *mask,
                              size_t                mask_buffer_size,
                              uint32_t             *mask_len_bit);

/**
 * @brief       Set the channel mask bitwise
 * @param mask  Technically would work with multiple regions,
 *              but currently only supports ETSI lower band
 *              because of restrictions. Max value therefor is 0x0F (4 channels set)
 * @return      POSIX Error Code
 */
int mt_uhf_set_channel_mask(uint64_t mask);

/**
 * @brief       Get the current channel mask, mirroring mt_uhf_set_channel_mask()
 * @param mask  Pointer to put the value
 * @return      POSIX Error Code
 */
int mt_uhf_get_channel_mask(uint64_t *mask);

#if MT_UHF_GPI_COUNT
/**
 * @brief           Get the state of all inputs
 * @param buffer    Target buffer for the values
 * @return          POSIX Error Code
 */
int mt_uhf_get_input(mt_uhf_input_states_t buffer);
#endif

#if MT_UHF_GPO_COUNT
/**
 * @brief           Set the state of all outputs, you can use mt_uhf_io_state_no_change
 *                  to not change some.
 * @param buffer 
 * @return 
 */
int mt_uhf_set_output(mt_uhf_output_states_t buffer);
#endif

/**
 * @brief  Function which needs to be called regularly to parse event data while no command is running,
 *         for example continious inventories, heartbeats or input events.
 *         Checks the internal character buffer and updates the internal statemachine, calling callbacks if required.
 *         Shouldn't be called from an interrupt context! 
 *         This function isn't required to be called if no events are active.
 * @return 0 on success
 *         -EAGAIN  if current command handling on current data hasn't finished yet
 *         -ENODATA if the current data isn't enough for more handling
 *         -EFAULT  on device error
 *         -EBADMSG on communication error 
 */
int mt_uhf_resolve(void);

//TODO: Add FDB, PLY, SUS, HOT, IEV, IP4, PMACT, SRACT commands