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
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "tag.h"

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
 * @brief The fuction type for frame data elements 
 * (lines usually starting with a '+', prefix seperated from data by a colon)
 */
typedef int (*mt_uhf_data_cb_t)(const char *prefix, char *data, bool finalized);

/** 
 * @brief   The type of fuction to call when a tag is found. 
 *          Should accept call with NULL to represent end of round
 */
typedef bool (*mt_uhf_tag_cb)(struct mt_uhf_gen2_tag *tag);

/** 
 * @brief   The type to get called to fill up the rx buffer
 */
typedef int (*mt_uhf_poll_cb)(void);

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
 * @brief A type for callbacks to give inventory data from one antenna over
 */
struct mt_uhf_inventory_buffer {
    struct {
        uint16_t                size;
        uint16_t                fill;
        uint32_t                found;
        struct mt_uhf_gen2_tag *buffer;
    } tags;
    uint8_t antenna; //will be 0 if unknown, for example in  INVR answer
};

/**
 * @brief Containing the reader identification data like names, revision and serial number
 */
struct mt_uhf_reader_identification {
    uint32_t known_parts; //5 bit used for the forset ID parts, can be increased in the future

    uint8_t fw_name[32];
    uint8_t fw_rev[5];
    uint8_t hw_name[32];
    uint8_t hw_rev[5];
    uint8_t serial[17];
};

/**
 * @brief The target bank of read / write operations
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

/**
 * @brief Part of the selection
 */
struct mt_uhf_select_element {
    enum mt_uhf_mem_bank bank;
    unsigned             target_address_bit;
    uint8_t             *data;
    unsigned             data_len_bit;
};

/**
 * @brief   Allows to describe a multi step selction
 *          target is not needed, it should always be B for 
 *          INV setting in select mode (and moving to be during select)
 */
struct mt_uhf_select_data {
    /** will switch session if reader is not correct already */
    enum mt_uhf_session session;
    /** A pointer to the elements */
    struct mt_uhf_select_element *elements;
    /** The number of elements in member elements */
    unsigned select_element_count;
};