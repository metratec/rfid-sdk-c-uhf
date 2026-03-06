/**
 * @file: reader.h                                                                                 *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_INTERN_READER_H
#define MT_UHF_SDK_INTERN_READER_H

//First include types including config
#include <metratec/uhf_reader/intern/common.h>

#define DEFINE_PREFIX(x) static const char prefix_##x[] = "+" #x

struct mt_uhf_frame_data_cb_lookup {
    const char      *id;
    mt_uhf_data_cb_t cb;
};

/** @brief      Info which values are expected in inventory answers
 *  @details    Order in answer: EPC, TID, RSSI, Phase (in 2 int)
 *              All values are bit values so they can be stacked
 */
enum uhfv2_inventory_tag_data_format {
    uhfv2_inventory_tag_data_format_unknown = 0,
    uhfv2_inventory_tag_data_format_epc     = 0x01,
    uhfv2_inventory_tag_data_format_TID     = 0x02, //set by INVS
    uhfv2_inventory_tag_data_format_RSSI    = 0x04, //set by INVS
    uhfv2_inventory_tag_data_format_Phase   = 0x08, //set by INVS
};
enum uhf_resp_state { uhf_resp_state_start, uhf_resp_state_echo, uhf_resp_state_end };

/**
 * @brief Data structure to hold the reader state and settings informations
 */
struct uhfv2_reader {
    struct {
        struct mt_uhf_reader_identification           data;
        enum mt_uhf_reader_identification_known_parts known_parts;
    } id;
    struct uhfv2_reader_state {
        //use none as not tried, true as successful, false as failed and invalid as in progress
        mt_uhf_boolx_t                       initialized;
        mt_uhf_boolx_t                       echo_enabled;
        enum mt_uhf_rf_mode                  rf_mode;
        enum mt_uhf_region                   region;
        enum mt_uhf_session                  session;
        enum uhfv2_inventory_tag_data_format inv_format;
        mt_uhf_boolx_t                       only_new_tag;
        mt_uhf_boolx_t                       fast_start;
        enum mt_uhf_gen2_inventory_target    inv_target;
        enum mt_uhf_gen2_inventory_selecting inv_selected;
    } state;
    struct mt_uhf_frame_data_cb_lookup data_cb[MT_UHF_MAX_EVENT_TYPES];

    mt_uhf_unexpected_frame_cb_t unexpected_frame_cb;
    //used for unknown but valid data, usually for answer
    mt_uhf_data_cb_t unknown_data_cb;
    void (*cinv_round_cb)(struct mt_uhf_inventory_buffer *inv_buf);
    struct {
        mt_uhf_tag_cb                   cb;
        struct mt_uhf_gen2_tag          last;
        struct mt_uhf_inventory_buffer *inv_buf;
        uint8_t                         antenna;
    } tag;

    struct {
        mt_uhf_poll_cb_t poll_cb;
    } rx;

    struct {
        const char                        *data; //string containing the last command, used for echo
        uint32_t                           start_time;
        uint32_t                           timeout;
        struct mt_uhf_frame_data_cb_lookup rsp_data;
        void                              *usr_data;
        mt_uhf_blocking_cb_t               blocking_cb;
    } cmd;
    struct {
        struct {
            struct uhfv2_frame                  current, last;
            struct mt_uhf_frame_data_cb_lookup *last_data_match;
        } frame;
        enum uhf_resp_state state;
        bool                running_cinv;
        struct {
            uint32_t base;
            uint32_t cinv_running;
        } default_timeout;
    } resp;
};
struct mt_uhf_byte_array {
    uint8_t *data;
    size_t   data_len;
};

/**
 * @brief Access to the reader internal data structure
 */
extern struct uhfv2_reader reader;

/**
 * @brief   Access to the strings to set selection targets
 */
extern const char *selected_strings[mt_uhf_gen2_inventory_selecting_length];

/**
 * @brief           Basic handler function for setter functions
 * @details         This expects the reader function returning nothing but "OK" or "ERROR"
 *                  Every non matching answer will be reported as an error
 * 
 * @param cmd       The command to send as a string
 * @param timeout   The timeout value, use 0 to use the default timeout
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */

mt_uhf_errorcode_t mt_uhf_setter_call(const char *cmd, uint32_t timeout);

/**
 * @brief           Generic call for a data getting function
 * 
 * @param cmd       The command as a string, needs to start with AT and will be parsed
 * @param data_cb   A callback function to give the data event to
 * @param usr_data  A pointer to data that may be used by the callback function (for example to hold buffers)
 * @param timeout   The timeout of the call, set to 0 for default value
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t
mt_uhf_get_data(const char *cmd, mt_uhf_data_cb_t data_cb, void *usr_data, uint32_t timeout);

/**
 * @brief           A generic call to get a byte array answer from the reader
 * 
 * @param name      Name of the function, AT+ gets added by the function
 * @param usr_data  A byte array to put the received data to
 * @param timeout   The timeout of the call, set to 0 for default value
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_data_byte_array(const char               *name,
                                              struct mt_uhf_byte_array *usr_data,
                                              uint32_t                  timeout);

/**
 * @brief       Set the polling callback function
 * 
 * @param cb    A callback function that must be non blocking
 *              Will be called by the SDK to get data
 *              Can be NULL to disable polling
 */
void mt_uhf_set_polling(mt_uhf_poll_cb_t cb);

/**
 * @brief           Function to resolve the current function
 * 
 * @param block     Set if blocking call. If not blocking it needs to be re-called regurarly
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_response_resolve(const bool block);

/**
 * @brief           Reset the device
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_device_reset(void);

/**
 * @brief           Update the reader identification (rev, name, sn)
 * @details         Data will be available in reader struct
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_update_reader_identification(void);

/**
 * @brief           Set the echo mode (on / off)
 * @details         will do it even if it thinks it's already in the state
 *                  so it can be used to fix an issue
 * 
 * @param enable    True for enabling, false for disabling
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_reader_echo_set(bool enable);

/**
 * @brief           Get the current echo state
 * @details         Will usually be done by getting the current state
 *                  If that's not known / invalid tries to update it first
 * 
 * @return          Returns a pointer to the current state (data may change later!)
 *                  or NULL if the state is not known and the device did not answer properly
 */
mt_uhf_boolx_t *mt_uhf_reader_echo_get(void);

/**
 * @brief           Check if a mode is valid for the device
 * 
 * @param mode      The mode to check
 * 
 * @return          True if valid, else false
 */
bool mt_uhf_rf_mode_valid(enum mt_uhf_rf_mode mode);

/**
 * @brief           Update the stored value of the rf mode
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_update_rf_mode(void);

/**
 * @brief           Update the stored value of the region
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code 
 */
mt_uhf_errorcode_t mt_uhf_update_region(void);

/**
 * @brief           Get a string for a region code (the string that's used by the reader!)
 * 
 * @param region    Region code
 *  
 * @return          C string, zero terminated
 * @returns         NULL for invalid region
 */
const char *mt_uhf_get_region_name(enum mt_uhf_region region);

/**
 * @brief           Check if a mode is valid for a specific region
 * 
 * @param region    The region to check for
 * @param rf_mode   The mode that needs checking
 * 
 * @return          True if the mode is ok for the region, else false
 */
bool mt_uhf_mode_matches_region(enum mt_uhf_region region, enum mt_uhf_rf_mode rf_mode);

/**
 * @brief           Update the stored value of the session
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code 
 */
mt_uhf_errorcode_t mt_uhf_update_session(void);

/**
 * @brief           Get a string for a region code (the string that's used by the reader!)
 * 
 * @param mem_bank  Memory bank code
 * 
 * @return          C string, zero terminated
 * @returns         NULL for invalid memory bank
 */
const char *mt_uhf_get_membank_name(enum mt_uhf_mem_bank mem_bank);

/**
 * @brief           Update the stored value of echo
 * 
 * @return          mt_uhf_errorcode_success (0) on success
 * @returns         < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_update_echo(void);

/**
 * @brief   A structure with all data potentially set by the AT+INVS command
 */
struct mt_uhf_invs_usr_data {
    bool                                 only_new_tags;
    bool                                 show_rssi;
    bool                                 read_tid;
    bool                                 fast_start;
    bool                                 show_phase;
    enum mt_uhf_gen2_inventory_selecting selected;
    enum mt_uhf_gen2_inventory_target    target;
    int                                  rssi_threshold;
};

/**
 * @brief   A structure with all data potentially set by the AT+BMSK command
 */
struct mt_uhf_bmsk_usr_data {
    enum mt_uhf_mem_bank *target;
    uint32_t             *mask_start_bit;
    uint32_t             *mask_len_bit;
    uint8_t              *mask;
    size_t                mask_buffer_size;
};

/**
 * @brief               A function provided as callback for AT+BMSK?
 * 
 * @param prefix        The prefix expected, will be tested to be +BMSK
 * @param data          The data part of the answer
 * @param finalized     If it is the last data (should be, it's a one line answer)
 * 
 * @return              mt_uhf_errorcode_success (0) on success
 * @returns             < 0: Error code
 */
mt_uhf_errorcode_t mt_uhf_get_inventory_mask_cb(const char *prefix, char *data, bool finalized);

#endif //include guard