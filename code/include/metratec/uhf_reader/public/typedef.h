/**
 * @file: typedef.h                                                                                *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-29                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 *                                                                                                 *
 * @brief Provides the needed types for the reader access, either by include of headers or directly
 */

#ifndef MT_UHF_SDK_PUBLIC_TYPEDEF_H
#define MT_UHF_SDK_PUBLIC_TYPEDEF_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//Config first
#include <metratec/uhf_reader/config.h>

//Then functions that may rely on config
#include <metratec/uhf_reader/public/errorcodes.h>
#include <metratec/uhf_reader/public/tag.h>

/**
 * @brief       A callback intended to be called when a frame is totally 
 *              unexpected (no command running, no event type ('+' start))
 * @details     The return value usually shall be mt_uhf_errorcode_no_data_available.
 *              This will make the sdk handle the frame as invalid (just as if the
 *              callback isn't set at all (NULL)).
 *              Any other return values will make mt_uhf_framehandler() return it and
 *              not handle the frame as invalid, therefor giving over handling to the user
 * @warning     Returning anything but mt_uhf_errorcode_no_data_available might cause unexpected behaviour!
 */
typedef mt_uhf_errorcode_t (*mt_uhf_unexpected_frame_cb_t)(const char *);

/**
 * @brief       A callback intended to be called if the sdk is called blocking and there's no data to handle
 * @details     This can be used to have a custom amount of waiting, to use events or
 *              handle other code in pauses. It's up to the user to decide what's done but
 *              it should not block too long as polling or handling on data is still needed to continue.
 */
typedef void (*mt_uhf_blocking_cb_t)(void);

/** 
 * @brief Generic buffer type, line buffer, can be used to transfer data information into callbacks
 */
struct mt_uhf_buffer {
    uint8_t *data;
    uint16_t fill, size;
};

/** 
 * @brief Like mt_uhf_buffer but with inherent buffer for the EPC and therefor no size needed
 */
struct mt_uhf_buffer_epc {
    uint8_t  data[MT_UHF_GEN2_MAX_EPC_BYTES];
    uint16_t fill;
};

/** 
 * @brief Like mt_uhf_buffer but with inherent buffer for the TID and therefor no size needed
 */
struct mt_uhf_buffer_tid {
    uint8_t  data[MT_UHF_GEN2_MAX_TID_BYTES];
    uint16_t fill;
};

/**
 * @brief Representation of a queried GEN2 Tag
 * 
 * The SDK can give them as parameter for a callback or put them into inventory buffer
 * It's up to the used to copy keep them if needed 
 */
struct mt_uhf_gen2_tag {
    /** count will be 1 after "normal" EPC, can be higher in reported INV
     *  Count will NOT count up in multiplexed inv, even if the same tag gets found on multiple antennas
     */
    uint32_t count;
    /** There is always an EPC (but it can have length 0!) */
    struct mt_uhf_buffer_epc epc;
    /** TID information is optional, if non is retreaved the fill is 0 */
    struct mt_uhf_buffer_tid tid;
    /** optional Signal strength indicator in dBm, it's 0 for none */
    int32_t rssi;
    /** optional phase info, it's zero for none */
    int32_t phase[2];
};

/** 
 * @brief           The fuction type for frame data elements
 *  @note           (lines usually starting with a '+', prefix seperated from data by a colon)
 * 
 * @param prefix    A string with the prefix text as sometimes multiple prefixes may use the same callback
 * @param data      The data part (after colon)
 * @param finalized Informing if the packet is finished with LF or more lines are expected
 * 
 * @returns         mt_uhf_errorcode_success (0) on success
 * @returns         Negatives for errors
 */
typedef mt_uhf_errorcode_t (*mt_uhf_data_cb_t)(const char *prefix, char *data, bool finalized);

/** 
 * @brief   The type of fuction to call when a tag is found. 
 * 
 * @param   tag     Pointer to tag data, must accept call with NULL to represent end of round
 * 
 * @returns         Error code, zero if successful
 */
typedef bool (*mt_uhf_tag_cb)(struct mt_uhf_gen2_tag *tag);

/** 
 * @brief           The type to get called to fill up the rx buffer, must be non blocking
 * 
 * @returns         Amount of data (>=0) on success
 * @returns         Negatives for errors, mt_uhf_errorcode_no_data_available is not allowed, use 0 instead
 */
typedef mt_uhf_errorcode_t (*mt_uhf_poll_cb_t)(void);

/** 
 * @brief RF modes list, 0 is used as unknown to default to it 
 */
enum mt_uhf_rf_mode {
    mt_uhf_rf_mode_unknown = 0,
    mt_uhf_rf_mode_103     = 103,
    mt_uhf_rf_mode_302     = 302,
    mt_uhf_rf_mode_120     = 120,
    mt_uhf_rf_mode_323     = 323,
    mt_uhf_rf_mode_345     = 345,
    mt_uhf_rf_mode_344     = 344,
    mt_uhf_rf_mode_223     = 223,
    mt_uhf_rf_mode_222     = 222,
    mt_uhf_rf_mode_241     = 241,
    mt_uhf_rf_mode_244     = 244,
    mt_uhf_rf_mode_285     = 285,
};

/** 
 * @brief Standard regions list, 0 is used as unknown to default to it 
 */
enum mt_uhf_region {
    mt_uhf_region_none_unknown   = 0,
    mt_uhf_region_etsi           = 0x01,
    mt_uhf_region_etsi_lowerband = mt_uhf_region_etsi,
    mt_uhf_region_etsi_upperband = 0x02,
    mt_uhf_region_fcc            = 0x04,
    mt_uhf_region_prc            = 0x08,
    mt_uhf_region_all            = 0x0F,
};

/** 
 * @brief Bool with more options
 */
typedef enum mt_uhf_boolx {
    mt_uhf_boolx_none = 0, //either unknown or, for write, don't change
    mt_uhf_boolx_false,
    mt_uhf_boolx_true,
    mt_uhf_boolx_invalid //check value range
} mt_uhf_boolx_t;

#if MT_UHF_GPI_COUNT
/**
 * @brief All GPIO input states of the device, only exists for devices with inputs
 */
typedef mt_uhf_boolx_t mt_uhf_input_states_t[MT_UHF_GPI_COUNT];
#endif
#if MT_UHF_GPO_COUNT
/**
 * @brief All GPIO output states of the device, only exists for devices with outputs
 */
typedef mt_uhf_boolx_t mt_uhf_output_states_t[MT_UHF_GPO_COUNT];
#endif

/**
 * @brief States of session setting on device
  */
enum mt_uhf_session {
    mt_uhf_session_unknown = -2,
    mt_uhf_session_auto_sl = -1,
    mt_uhf_session_s0      = 0,
    mt_uhf_session_s1      = 1,
    mt_uhf_session_s2      = 2,
    mt_uhf_session_s3      = 3,
    mt_uhf_session_max     = mt_uhf_session_s3,
};

/**
 * @brief States of select setting on device
 */
enum mt_uhf_gen2_inventory_selecting {
    mt_uhf_gen2_inventory_selecting_not_selected = 0,
    mt_uhf_gen2_inventory_selecting_selected,
    mt_uhf_gen2_inventory_selecting_all,
    mt_uhf_gen2_inventory_selecting_length
};

/**
 * @brief States of target setting on device
 */
enum mt_uhf_gen2_inventory_target {
    mt_uhf_gen2_inventory_target_A = 0,
    mt_uhf_gen2_inventory_target_B,
    mt_uhf_gen2_inventory_target_length,
};

/**
 * @brief   A type for callbacks to give inventory data from one antenna over
 * @struct  mt_uhf_inventory_buffer
 */
struct mt_uhf_inventory_buffer {
    /**
     * @brief   This structure contains informations about the tags
     */
    struct {
        uint16_t size;  ///size of buffer in tags
        uint16_t fill;  ///amount of tag buffer fill, <= found, <= size
        uint32_t found; ///The amount of tags found. Tags get buffered as long as fill < size
        struct mt_uhf_gen2_tag *buffer; ///A buffer to put tags into, space for <size> tags
    } tags;
    uint8_t antenna; ///ID of the antenna finding the tags, will be 0 if unknown, e.g. in INVR answer
};

/** 
 * @brief           The type to get called by contious inventory when a round is done
 * 
 * @param inv_buf   Gets a pointer to the buffer with the found data, this can be NULL (was provided by user at inventory call)
 */
typedef void (*cinv_round_done_cb_t)(struct mt_uhf_inventory_buffer *inv_buf);

/**
 * @brief   Containing the reader identification data like names, revision and serial number
 * 
 * @struct  mt_uhf_reader_identification
 */
struct mt_uhf_reader_identification {
    char fw_name[32]; ///Name of reader firmware as a string with max length 32 (then it's no C string)
    char fw_rev[5]; ///Firmware revision in the 4 char MKU format + '\0'
    char hw_name[32]; ///Name of reader hardware as a string with max length 32 (then it's no C string)
    char hw_rev[5];  ///Hardware revision in the 4 char MKU format + '\0'
    char serial[17]; ///The reader device's serial number in 16 byte char MKU format + '\0'
};

enum mt_uhf_reader_identification_known_parts {
    mt_uhf_reader_identification_known_parts_none          = 0,
    mt_uhf_reader_identification_known_parts_sw_name       = 0x01,
    mt_uhf_reader_identification_known_parts_sw_rev        = 0x02,
    mt_uhf_reader_identification_known_parts_hw_name       = 0x04,
    mt_uhf_reader_identification_known_parts_hw_rev        = 0x08,
    mt_uhf_reader_identification_known_parts_serial_number = 0x10,
    //all firmware infos
    mt_uhf_reader_identification_known_parts_sw = mt_uhf_reader_identification_known_parts_sw_name +
                                                  mt_uhf_reader_identification_known_parts_sw_rev,
    //all hardware infos
    mt_uhf_reader_identification_known_parts_hw = mt_uhf_reader_identification_known_parts_hw_name +
                                                  mt_uhf_reader_identification_known_parts_hw_rev,
    //all expected data
    mt_uhf_reader_identification_known_parts_all =
        mt_uhf_reader_identification_known_parts_sw + mt_uhf_reader_identification_known_parts_hw +
        mt_uhf_reader_identification_known_parts_serial_number
};

/**
 * @brief   The target bank of read / write operations
 */
enum mt_uhf_mem_bank {
    mt_uhf_mem_bank_EPC   = 0,
    mt_uhf_mem_bank_first = 0,
    mt_uhf_mem_bank_PC,
    mt_uhf_mem_bank_TID,
    mt_uhf_mem_bank_USR,
    mt_uhf_mem_bank_OFF,
    mt_uhf_mem_bank_length,
    mt_uhf_mem_bank_none = mt_uhf_mem_bank_length
};

#endif //include guard